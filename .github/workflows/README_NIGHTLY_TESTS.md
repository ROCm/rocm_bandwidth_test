# RBT Nightly Tests Workflow

This document describes [`.github/workflows/rbt-nightly-tests.yml`](./rbt-nightly-tests.yml),
which picks up the **latest RBT tarball** from the index URL configured in
`vars.RBT_TARBALL_INDEX_URL` (e.g. `https://repo.amd.com/rocm/rbt/tarball/`)
once per day, copies it to a **configurable remote target node** over SSH,
installs it there, and runs bandwidth tests (healthcheck, P2P, topology) on that node.

The GitHub Actions runner ("RBT Runner") is only an **orchestrator** — it
doesn't need a GPU or ROCm installed locally. All RBT install, binary
verification, and test execution happens on the target node. The target node
is configurable so the same workflow can be pointed at any GPU host without
code changes.

## What it does

```
schedule / workflow_run / manual
    │
    ▼
detect-tarball              [utility runner]
    │  curl $RBT_TARBALL_INDEX_URL → pick newest amdrocm*-rbt-*.tar.gz
    ▼
prepare-test-context        [utility runner]
    │  rbt_nightly_test.sh validate-config → paths + remote work dir
    ▼
install-rbt-on-target       [test runner]  ──ssh──▶  [target GPU node]
    │  rbt_nightly_test.sh: setup-ssh, verify-rocm, download, copy,
    │                        install-rbt, verify-rbt-binary
    ▼
run-rbt-tests               [test runner]  ──ssh──▶  [target GPU node]
    │  rbt_nightly_test.sh: run-tests (healthcheck, p2p, topology),
    │  collect-logs, capture-versions
    │  upload intermediate logs artifact; cleanup remote work dir
    ▼
create-test-report          [utility runner]
    │  rbt_nightly_test.sh build-report → SUMMARY.md + final artifact
    ▼
artifact: rbt-nightly-report-<run_id>
```

Install, test, and report logic lives in [`rbt_nightly_test.sh`](../../rbt_nightly_test.sh)
at the repo root — the same split as `build-relocatable-packages.yml` +
`build_packages_local.sh`.

## Triggers

| Trigger | Cadence | What fires |
|---|---|---|
| `schedule` | `0 15 * * *` UTC daily (08:00 PST / 07:00 PDT) | Polls the tarball index and always runs install + tests, even when the latest tarball filename matches the previous run (a notice is logged when unchanged). |
| `workflow_run` | After **Build Relocatable Packages** completes | Runs only when that workflow's overall conclusion is **success**. No changes to the build workflow are required. |
| `workflow_dispatch` | Manual | Always runs. Supports overriding the tarball URL and **retargeting at any node** without editing the workflow. |

The cron deliberately runs after AMD's typical nightly publish window;
adjust the cron string in the workflow if your publish cadence is different.

## Manual dispatch inputs

| Input | Default | Description |
|---|---|---|
| `tarball_url` | _(empty)_ | If set, the workflow downloads this exact URL instead of scraping the index. Useful for re-running an older build. |
| `target_node` | _(empty → `secrets.RBT_TARGET_NODE`)_ | Hostname or IP of the node to install RBT on and run tests against. **This is the value that retargets the test execution.** Stored as a secret so the lab node identity isn't visible in repo settings or run logs (GitHub Actions automatically masks secret values as `***` in step output). |
| `target_user` | _(empty → `secrets.RBT_TARGET_USER`; if both are unset, SSH defaults to the orchestrator runner's local user)_ | SSH user on the target node. Must have `NOPASSWD` sudo on the target (see prerequisites). Stored as a secret so the lab account name isn't visible in repo settings or run logs (GitHub Actions automatically masks secret values as `***` in step output). |
| `remote_work_dir` | _(empty → `vars.RBT_REMOTE_WORK_DIR`, then `/tmp/rbt-nightly-<run_id>`)_ | Working dir on the target node where the tarball is staged, logs are written, and which gets `rm -rf`'d at the end. |
| `target_rocm_path` | _(empty → `vars.RBT_TARGET_ROCM_PATH`)_ — **required**, no hard-coded default | Absolute path to the ROCm tarball install root on the target node. This is the directory containing `bin/rocminfo`, `bin/amd-smi`, `lib/`, `lib/llvm/lib/`, and `lib/rocm_sysdeps/lib/` (the layout produced by the ROCm tarball install method). The workflow fails fast in the validate step if neither the input nor the variable is set. |

Workflow inputs **win over repo variables**, so individual `workflow_dispatch`
runs can be retargeted from the Actions UI without changing repo settings.

Example — point a single run at a specific node:

```bash
gh workflow run rbt-nightly-tests.yml \
  -f tarball_url="<INDEX_URL>/amdrocm7-rbt-2.6.2-r0711.20260609-Linux.tar.gz" \
  -f target_node="<HOST_OR_IP>" \
  -f target_user="<USER>"
```

Example — keep the default tarball but run on a different host today:

```bash
gh workflow run rbt-nightly-tests.yml -f target_node="<HOST_OR_IP>"
```

Example — point at a specific ROCm install on a multi-ROCm host:

```bash
gh workflow run rbt-nightly-tests.yml \
  -f target_node="<HOST_OR_IP>" \
  -f target_rocm_path="<ROCM_INSTALL_PATH>"
```

## Repository configuration

**Variables** (Settings → Secrets and variables → Actions → Variables):

| Name | Required? | Purpose |
|---|---|---|
| `RBT_TARBALL_INDEX_URL` | **Required** | Directory listing scraped for the latest tarball, e.g. `https://repo.amd.com/rocm/rbt/tarball/`. No fallback — the workflow fails fast if unset and no `tarball_url` input is supplied. |
| `RBT_REMOTE_WORK_DIR` | optional (default `/tmp/rbt-nightly-<run_id>`) | Working dir on the target node. Cleared with `rm -rf` at the end of the job. |
| `RBT_TARGET_ROCM_PATH` | **Required** *(unless every run sets `target_rocm_path` input)* | Absolute path to the ROCm tarball install root on the target node — the directory that contains `bin/rocminfo`, `bin/amd-smi`, `lib/`, `lib/llvm/lib/`, and `lib/rocm_sysdeps/lib/`. The workflow doesn't assume any conventional path (no `/opt/rocm` default), since tarball installs land wherever you extracted them. |
| `RBT_TEST_RUNNER_LABEL` | optional (default `self-hosted`) | Label of the GitHub runner that orchestrates the workflow. The runner doesn't need a GPU or ROCm — it just needs `ssh`, `scp`, and `curl`. |

**Secrets** (Settings → Secrets and variables → Actions → Secrets):

| Name | Required? | Purpose |
|---|---|---|
| `RBT_TARGET_NODE` | **Required** *(unless every run sets `target_node` input)* | Hostname or IP of the node where RBT is installed and tests run. Stored as a secret so the lab node identity isn't visible in repo Variables or in run logs — GitHub Actions automatically masks secret values as `***` wherever they appear in step output. Workflow fails fast on `schedule` if neither this secret nor `target_node` input is set. |
| `RBT_TARGET_USER` | optional (no hard-coded default) | SSH user on the target node. If unset and `target_user` input is empty, the SSH client falls back to the orchestrator runner's local user — set this secret explicitly to avoid surprises. Stored as a secret so the lab account name isn't visible in repo settings or run logs (auto-masked as `***`). |
| `RBT_TARGET_SSH_KEY` | **Required** | Private SSH key (OpenSSH or PEM format) authorized on the target node for `RBT_TARGET_USER`. Written to `$RUNNER_TEMP/rbt_target_key` for the duration of the job and scrubbed in the cleanup step. |

## How the latest tarball is picked

The `detect-tarball` job does (with `$INDEX_URL` = `vars.RBT_TARBALL_INDEX_URL`):

```bash
curl -sL "$INDEX_URL" \
  | grep -oE 'amdrocm[0-9]*-rbt-[0-9A-Za-z._\-]+-Linux\.tar\.gz' \
  | sort -uV \
  | tail -n 1
```

The regex matches any `amdrocm<N>-rbt-…-Linux.tar.gz` filename in the
directory-listing HTML. `sort -V` is GNU "version sort" so version
suffixes like `2.6.2-r0711-9` and `2.6.2-r0711-100` compare correctly. The
**lexicographically largest by version** is selected.

## How the tarball is installed (on the target node)

The runner derives the ROCm major version from the tarball filename
(e.g. `amdrocm7-rbt-2.6.2-r0711.20260609-Linux.tar.gz` → `7`), combines it with
`TARGET_ROCM_PATH` (which selects *which* ROCm install to use), writes
the derived paths to `$GITHUB_ENV`, then SSHes into the target with those
values exported so the install runs against the matching `extras-<major>`
directory under the chosen ROCm. The same workflow handles ROCm 6, 7,
etc., and any version inside `7.x` without code changes:

```bash
# On the runner (validate-config step):
# Parsed from $TARBALL_NAME via [[ "$TARBALL_NAME" =~ ^amdrocm([0-9]+)- ]]
ROCM_MAJOR=7
# INSTALL_DIR is always /opt/rocm/extras-<major>/ — it's where the RBT
# tarball gets extracted. This is decoupled from TARGET_ROCM_PATH so the
# install location matches the manual command verbatim regardless of
# which ROCm install the workflow is told to run *against*.
INSTALL_DIR=/opt/rocm/extras-${ROCM_MAJOR}              # /opt/rocm/extras-7
RBT_BIN=${INSTALL_DIR}/bin/rocm_bandwidth_test

# TARGET_ROCM_PATH (from inputs.target_rocm_path / vars.RBT_TARGET_ROCM_PATH;
# required, no default) is *where the ROCm runtime libraries live*. Set
# the repo variable to the absolute install root of the ROCm tarball you
# want RBT to run against (the directory that contains bin/, lib/, etc.).

# On the target node (install-rbt step), via SSH:
# sudo is used only when $INSTALL_DIR isn't user-writable. /opt/rocm/* is
# typically root-owned so this picks up sudo -n automatically.
mkdir -p "$INSTALL_DIR"     # (with `sudo -n` if needed)
tar -xzf "$REMOTE_WORK_DIR/pkg/<tarball>.tar.gz" -C "$INSTALL_DIR"

# LD_LIBRARY_PATH is wired off INSTALL_DIR (for RBT's own libs) plus the
# three TheRock-style subdirs of TARGET_ROCM_PATH (matches the official
# ROCm tarball-install docs). This is what makes a non-/opt/rocm
# TARGET_ROCM_PATH actually do something useful.
export LD_LIBRARY_PATH="${INSTALL_DIR}/lib:${TARGET_ROCM_PATH}/lib/rocm_sysdeps/lib:${TARGET_ROCM_PATH}/lib/llvm/lib:${TARGET_ROCM_PATH}/lib:${LD_LIBRARY_PATH:-}"

"$RBT_BIN" --version
```

### Install location vs. ROCm runtime path

Two paths in the workflow look similar but mean very different things, and
keeping them straight is what makes the multi-ROCm-host case work:

| Variable | What it is | Default | How to override |
|---|---|---|---|
| `INSTALL_DIR` | Where the RBT tarball gets extracted on the target node. Always under `/opt/rocm/`, matching the manual command. | `/opt/rocm/extras-${ROCM_MAJOR}` | Not configurable by design — the install location is canonical, decoupled from where ROCm itself lives. |
| `TARGET_ROCM_PATH` | Where the ROCm runtime libraries live (the directory containing `bin/`, `lib/`, `lib/llvm/lib/`, `lib/rocm_sysdeps/lib/`). Drives `LD_LIBRARY_PATH`, the prereq-check probe, and the version row in the report. | **(no default — required)** | `inputs.target_rocm_path` (workflow_dispatch) or `vars.RBT_TARGET_ROCM_PATH` (repo Variables tab). Workflow fails fast in the validate step if neither is set. |

This split exists because hosts that have multiple ROCm installs side-by-side
(or that installed ROCm via a TheRock tarball under `$HOME`) almost never
keep the runtime libs at `/opt/rocm/lib/`. The workflow needs to know where
`/lib/`, `/lib/llvm/lib/`, and `/lib/rocm_sysdeps/lib/` actually are; the
install location of RBT itself should not — and does not — care.

The runtime `LD_LIBRARY_PATH` for every step that executes `rocm_bandwidth_test`
(install, ldd-verify, tests, report `--version` query) is:

```bash
LD_LIBRARY_PATH=$INSTALL_DIR/lib:$TARGET_ROCM_PATH/lib/rocm_sysdeps/lib:$TARGET_ROCM_PATH/lib/llvm/lib:$TARGET_ROCM_PATH/lib:$LD_LIBRARY_PATH
```

### TheRock-style tarball ROCm installs

The [official ROCm 7.12+ "tarball" install method](https://rocm.docs.amd.com/en/7.12.0-preview/install/rocm.html?fam=instinct&gpu=mi350x&os=ubuntu&os-version=24.04&i=tar)
doesn't drop ROCm under `/opt/rocm-<ver>/`. Instead you extract a
single distribution archive (e.g.
`therock-dist-linux-<arch>-dcgpu-<version>.tar.gz`) into an
arbitrary directory, set `ROCM_PATH=$(pwd)/install`, and source the
env. So the layout looks like:

```
/home/<user>/<wherever-you-ran-tar>/install/
├── bin/                     ← rocm-smi, rocminfo, hipcc, …
├── lib/
│   ├── rocm_sysdeps/lib/    ← ROCm runtime libs live HERE in TheRock builds
│   └── llvm/lib/
├── share/
└── …
```

This works with the workflow as-is, with two things to be aware of:

1. **Find the absolute path of the install on the target node** — there's no canonical location, you pick it at install time. Either ask whoever installed it, or search:
   ```bash
   ssh <USER>@<HOST_OR_IP> '
     for cand in ~/install ~/*/install ~/rocm*/install /opt/therock*/install; do
       [ -x "$cand/bin/rocminfo" ] && echo "  found ROCm install: $cand"
     done
   '
   ```
2. **Pass that absolute path as `target_rocm_path`.** Example:
   ```bash
   gh workflow run rbt-nightly-tests.yml \
     -f target_node=<HOST_OR_IP> \
     -f target_rocm_path=$HOME/<wherever-you-extracted>/install
   ```

What the workflow handles automatically for tarball installs:

- The install step **skips `sudo`** when the destination is user-writable (TheRock installs in `$HOME` don't need root); only system `/opt/rocm-*` installs trigger `sudo -n`.
- The prereq check invokes `${TARGET_ROCM_PATH}/bin/rocminfo` and `${TARGET_ROCM_PATH}/bin/amd-smi version` directly by absolute path. TheRock tarball binaries have RPATH/RUNPATH baked in relative to `${TARGET_ROCM_PATH}/lib`, so they resolve their own ROCm libs without needing `PATH` or `LD_LIBRARY_PATH` setup.

`sudo -n` is non-interactive — it fails fast instead of hanging if
`NOPASSWD` isn't configured. There's no PTY over the SSH channel anyway,
so an interactive sudo prompt would deadlock the job.

The step fails fast if the filename doesn't match `^amdrocm<digits>-`, or
if `$RBT_BIN` isn't executable after extraction.

### Prerequisites

**On the GitHub runner (orchestrator):**

- `ssh`, `scp`, `ssh-keyscan`, and `curl` on `PATH`.
- Network egress to:
  - the host serving `vars.RBT_TARBALL_INDEX_URL` (typically port 443),
  - the target node (port 22 — or wherever its sshd listens, configurable in the SSH config block of the workflow).
- The runner does **not** need a GPU, ROCm, or `sudo`.

**On the target node** (all enforced by the **Pre-flight ROCm checks** below — the workflow fails fast if any are missing):

- SSH server reachable from the runner, with the public counterpart of `secrets.RBT_TARGET_SSH_KEY` authorized for `$TARGET_USER`.
- `$TARGET_USER` has **`NOPASSWD` sudo** for `mkdir` + `tar` into `/opt/rocm/extras-<major>` — the install step uses `sudo -n` (when the path isn't user-writable) and aborts otherwise.
- A ROCm tarball install at `$TARGET_ROCM_PATH` (configured via `vars.RBT_TARGET_ROCM_PATH` or the per-run `target_rocm_path` input) that provides at least `bin/rocminfo` and `bin/amd-smi`. The RBT tarball only ships RBT, not the rest of ROCm, so the runtime libs under `$TARGET_ROCM_PATH/lib`, `$TARGET_ROCM_PATH/lib/llvm/lib`, and `$TARGET_ROCM_PATH/lib/rocm_sysdeps/lib` must be present.
- Working kernel driver (`amdgpu`) and at least one GPU enumerated by `rocminfo` / `amd-smi`.

## Pre-flight ROCm checks

Before downloading or installing the tarball, the workflow runs **`Verify ROCm prerequisites on target node`** over SSH. It's deliberately minimal — two probes against the actual install at `$TARGET_ROCM_PATH`:

| Sub-check | Action on failure | Catches |
|---|---|---|
| `$TARGET_ROCM_PATH` directory exists | hard fail (`exit 1`) | ROCm not installed on target, or the configured path is wrong (typo, missing trailing component, etc.) |
| `$TARGET_ROCM_PATH/bin/rocminfo` exits 0 | hard fail (via `set -euo pipefail`) | Driver not loaded, no GPU exposed, runtime libs under `$TARGET_ROCM_PATH/lib` missing/broken, group permissions wrong |
| `$TARGET_ROCM_PATH/bin/amd-smi version` exits 0 | hard fail | `amd-smi` missing from the install, ROCm SMI library mismatch, or driver/runtime broken |

Both binaries are invoked by absolute path (`${TARGET_ROCM_PATH}/bin/...`), so the workflow doesn't depend on `PATH` or `LD_LIBRARY_PATH` being set up — the binaries' baked-in RPATH/RUNPATH resolves the runtime libs from `${TARGET_ROCM_PATH}/lib`. If either probe fails, the step fails fast with the binary's own error message in the log, which is usually more diagnostic than anything the workflow could add on top.

A typical successful log (with `target_rocm_path=<ROCM_INSTALL_PATH>`):

```
=== System ===
Linux <hostname> 6.x.x-x-generic ...

=== Target ROCm path: <ROCM_INSTALL_PATH> ===

=== <ROCM_INSTALL_PATH>/bin/rocminfo ===
ROCk module version <X.Y> is loaded
HSA Agents
==========
Agent 1
  Name:                    AMD Instinct ...
  ...
Agent 2..N: (one per GPU)

=== <ROCM_INSTALL_PATH>/bin/amd-smi version ===
AMDSMI Tool: <X.Y.Z> | AMDSMI Library version: <X.Y.Z> | ROCm version: <ROCM_VERSION>

::notice::ROCm prerequisites OK on target node at <ROCM_INSTALL_PATH>
```

After install, **`Verify RBT binary library resolution on target node`** runs `ldd "$RBT_BIN"` over SSH (where `$RBT_BIN` = `/opt/rocm/extras-${ROCM_MAJOR}/bin/rocm_bandwidth_test`) and **hard-fails** the job if any library shows up as `not found`. This prevents the workflow from spending time on bandwidth tests only to discover a `dlopen` error in the log. The full `ldd` output is printed for diagnostic purposes.

### Manual one-liner (validate a candidate target node)

To verify a node is viable before pointing the workflow at it, SSH into the candidate and run the same two probes the workflow runs (substituting `<ROCM_INSTALL_PATH>` with what you plan to set `target_rocm_path` to):

```bash
ROCM_PATH=<ROCM_INSTALL_PATH>
set -euo pipefail
[ -d "$ROCM_PATH" ] || { echo "$ROCM_PATH does not exist"; exit 1; }
"$ROCM_PATH/bin/rocminfo"
"$ROCM_PATH/bin/amd-smi" version
sudo -n true 2>/dev/null && echo "NOPASSWD sudo OK" || echo "warn: sudo requires password (install step will fail)"
echo "Target node OK"
```

If both `rocminfo` and `amd-smi version` exit zero, the workflow's prereq step will pass on this node.

## The tests

Three tests run sequentially on the target node (with `$RBT_BIN` =
`/opt/rocm/extras-${ROCM_MAJOR}/bin/rocm_bandwidth_test`). Each is
executed over SSH with `set +e` so a failure in one test does not skip
the remaining ones — all three results are captured independently.

```bash
"$RBT_BIN" run transferbench healthcheck --verbose
"$RBT_BIN" run transferbench p2p --size=256MB --iterations=10
"$RBT_BIN" run transferbench topology
```

Each command's stdout/stderr is captured to a separate log file on the
target node (`healthcheck.log`, `p2p.log`, `topology.log`), then the
**`Collect logs from target node`** step `scp`s all logs back to `./reports/`
on the runner. Each exit code is propagated through SSH and recorded in a
step output. The workflow is marked failed in `create-test-report` if any
test exited non-zero.

## Test report

After tests finish and logs are collected, the `Build test report`
step generates `reports/SUMMARY.md` (also written to the GitHub job summary),
e.g.:

```markdown
# RBT Nightly Test Report

| Field | Value |
|---|---|
| Run | `1234567890` |
| Trigger | `schedule` |
| Target ROCm path | `<ROCM_INSTALL_PATH>` (version `<ROCM_VERSION>`) |
| Remote work dir | `/tmp/rbt-nightly-1234567890` |
| Tarball | `amdrocm7-rbt-2.6.2-r0711.20260609-Linux.tar.gz` |
| Source URL | `$RBT_TARBALL_INDEX_URL/amdrocm7-rbt-2.6.2-r0711.20260609-Linux.tar.gz` |
| RBT version | `rocm_bandwidth_test 2.6.2` |
| Overall result | **PASS** |

## Results

| Test         | Command                                                                    | Result | Exit | Started (UTC)        | Ended (UTC)          |
|--------------|----------------------------------------------------------------------------|:------:|-----:|----------------------|----------------------|
| Healthcheck  | `rocm_bandwidth_test run transferbench healthcheck`                        | PASS   |    0 | 2026-06-09T15:10:12Z | 2026-06-09T15:12:45Z |
| P2P Bandwidth| `rocm_bandwidth_test run transferbench p2p --size=256MB --iterations=10`   | PASS   |    0 | 2026-06-09T15:12:46Z | 2026-06-09T15:18:03Z |
| Topology     | `rocm_bandwidth_test run transferbench topology`                           | PASS   |    0 | 2026-06-09T15:18:04Z | 2026-06-09T15:19:22Z |
```

The full artifact contents:

```
rbt-nightly-report-<run_id>/
├── SUMMARY.md
├── healthcheck.log
├── p2p.log
└── topology.log
```

Artifact retention is 30 days.

## GitHub runner vs target node

The `run-rbt-tests` job runs on `${{ vars.RBT_TEST_RUNNER_LABEL || 'self-hosted' }}`,
but this runner only orchestrates — it doesn't need a GPU or ROCm. You can:

- Reuse an existing self-hosted runner that already has SSH access to the lab.
- Use a small purpose-built orchestration runner (any Linux box with `ssh`/`scp`/`curl`).
- In principle, use a GitHub-hosted `ubuntu-latest` runner — but that requires the target node to be reachable from GitHub's hosted runner IPs, which usually isn't the case for lab hosts behind a VPN/bastion.

The **target node** is what needs the GPU, ROCm, and `NOPASSWD` sudo. It does **not** need to be a registered GitHub runner.

If the chosen orchestrator runner is busy with another job, this workflow's
`concurrency:` group (`rbt-nightly-${{ github.workflow }}`,
`cancel-in-progress: false`) will queue the run rather than cancel the
running one.

## Verifying the pipeline end-to-end

After committing the workflow file to `amd-mainline`, the fastest sanity check
is:

```bash
gh workflow run rbt-nightly-tests.yml
```

Watch the Actions tab for:

1. `detect-tarball` resolves a tarball URL (`Latest tarball : amdrocm<N>-rbt-…`).
2. `prepare-test-context` job prints the resolved `Target node`, `Target ROCm path`, `Remote work dir`, and `Expected RBT binary` path.
3. `install-rbt-on-target` job picks up on your orchestrator runner.
4. **Setup SSH for target node** prints the target's `hostname` / `id` / `uptime` from the connectivity probe.
5. **Verify ROCm prerequisites on target node** prints `::notice::ROCm prerequisites OK on target node at <TARGET_ROCM_PATH>`.
6. **Install RBT on target node** prints the detected `ROCM_MAJOR`, the chosen `Target ROCm path`, and `Installed RBT at: /opt/rocm/extras-<N>/bin/rocm_bandwidth_test`.
7. **Verify RBT binary library resolution on target node** prints `::notice::RBT binary's library dependencies resolved OK on target.`
8. `run-rbt-tests` job: all three test steps complete; the run summary shows the results table with PASS/FAIL per test.

## Debugging a failed run

| Symptom | Likely cause |
|---|---|
| `detect-tarball` job exits with `vars.RBT_TARBALL_INDEX_URL is not set` | The required variable is unset. Set it in the repo Variables, or pass `tarball_url` via `workflow_dispatch`. |
| `detect-tarball` job exits with "Could not resolve a tarball URL" | The index page returned no matches. Verify the URL in `vars.RBT_TARBALL_INDEX_URL` returns at least one `amdrocm*-rbt-*-Linux.tar.gz` link. |
| `install-rbt-on-target` job stuck "Queued" | No orchestrator runner online with the label in `vars.RBT_TEST_RUNNER_LABEL`. |
| **validate-config** fails: `No target node configured` | Neither `inputs.target_node` nor `secrets.RBT_TARGET_NODE` is set. Add the secret in Settings → Secrets and variables → Actions → Secrets, or pass `target_node` via `workflow_dispatch`. |
| **setup-ssh** fails: `SSH_PRIVATE_KEY is not set` | The required secret `RBT_TARGET_SSH_KEY` is missing. Add the private SSH key as a repo secret. |
| **setup-ssh** fails: `Permission denied (publickey)` | The key in `RBT_TARGET_SSH_KEY` isn't authorized on the target node for `$TARGET_USER`, or the key format is wrong. Verify by `ssh -i <key> $TARGET_USER@$TARGET_NODE hostname` from a workstation. |
| **setup-ssh** fails: `Connection timed out` / `Connection refused` | Network reachability problem between the orchestrator runner and the target node. Check firewall / VPN / bastion routing. |
| **Verify ROCm prerequisites on target node** fails: `<TARGET_ROCM_PATH> does not exist on the target node` | The configured `target_rocm_path` / `vars.RBT_TARGET_ROCM_PATH` doesn't point at a real directory on the target. Verify by `ssh <USER>@<HOST_OR_IP> 'ls -d <TARGET_ROCM_PATH>'`. |
| **Verify ROCm prerequisites on target node** fails: `<TARGET_ROCM_PATH>/bin/rocminfo: No such file or directory` | The install at `$TARGET_ROCM_PATH` is incomplete — missing `bin/rocminfo`. Pick a different `target_rocm_path` or reinstall ROCm at the configured path. |
| **Verify ROCm prerequisites on target node** fails: `rocminfo` exits non-zero | Driver issue or runtime libs missing. Common causes: `amdgpu` kernel driver not loaded (`lsmod \| grep amdgpu`, then `sudo modprobe amdgpu`); `$TARGET_USER` not in `video`/`render` groups; `$TARGET_ROCM_PATH/lib` missing or broken. |
| **Verify ROCm prerequisites on target node** fails: `amd-smi version` exits non-zero | Either `amd-smi` is missing from `$TARGET_ROCM_PATH/bin/`, or the AMDSMI library under `$TARGET_ROCM_PATH/lib` is broken/missing. Reinstall or repoint `target_rocm_path` at a complete install. |
| **Verify RBT binary library resolution on target node** fails: `RBT binary has unresolved library dependencies on target` | One or more libraries showed up as `not found` in `ldd` output. `LD_LIBRARY_PATH` (built from `$INSTALL_DIR/lib` + `$TARGET_ROCM_PATH/{lib,lib/llvm/lib,lib/rocm_sysdeps/lib}`) didn't cover them. Usually means `$TARGET_ROCM_PATH` is incomplete. The step's `ldd` output names the specific missing library. |
| **Install RBT on target node** fails: `Cannot parse ROCm major version from tarball name` | The tarball doesn't match `^amdrocm<digits>-`. Either pin a correctly-named tarball with `tarball_url`, or fix the upstream filename. |
| **Install RBT on target node** fails: `sudo: a password is required` | `$TARGET_USER` doesn't have `NOPASSWD` sudo on the target. Add a sudoers entry permitting `mkdir` and `tar` into `/opt/rocm/extras-*` without a password. |
| **Install RBT on target node** fails: `rocm_bandwidth_test binary not found or not executable` | The tarball isn't rooted at `./bin/`, `./lib/`, etc. Inspect the step log (`ls -la` output) to see the actual layout. |
| **run-tests** healthcheck exits non-zero immediately (after both verify steps passed) | RBT plugin dependency missing on the target (e.g. `libnuma1`). Check `healthcheck.log` in the artifact for the specific error. |
| **Collect logs from target node** warns: `No log files retrieved from target node` | The test steps exited so early they didn't produce any output, or `$REMOTE_WORK_DIR` was wiped. Inspect the test-step logs in the run UI for the original error. |
| Cron skipped a day | GitHub may delay or drop schedules under high load. Run once manually via `workflow_dispatch` to validate. |

## Retargeting at a different node

There are three ways to point the workflow at a different node, in increasing order of permanence:

1. **Single run, from the Actions UI:** Run workflow → fill in `target_node` and `target_rocm_path` (and optionally `target_user` / `remote_work_dir`). Anything left blank falls back to the matching repo configuration value — `target_node` and `target_user` read from the **secrets** `RBT_TARGET_NODE` / `RBT_TARGET_USER` (both masked as `***` in run logs); `target_rocm_path` / `remote_work_dir` read from repo Variables. `target_node` and `target_rocm_path` have no hard-coded defaults — the workflow fails fast if neither input nor stored value is set for either.
2. **Single run, from `gh` CLI:**
   ```bash
   gh workflow run rbt-nightly-tests.yml \
     -f target_node="<host-or-ip>" \
     -f target_rocm_path="<ROCM_INSTALL_PATH>"
   ```
3. **Permanent change:** update repo secrets `RBT_TARGET_NODE` / `RBT_TARGET_USER` and repo variable `RBT_TARGET_ROCM_PATH` (and optionally repo variable `RBT_REMOTE_WORK_DIR`). All subsequent scheduled and manual runs will pick these up unless an input overrides them.

The same `RBT_TARGET_SSH_KEY` secret is reused across nodes — make sure the
public counterpart of that key is added to `$TARGET_USER`'s `~/.ssh/authorized_keys`
on **every** node you intend to point the workflow at.

## References

- [ROCm Bandwidth Test source](../../README.md)
- [`build-relocatable-packages.yml`](./build-relocatable-packages.yml) and [`README_BUILD_PACKAGES.md`](./README_BUILD_PACKAGES.md) — the upstream packaging pipeline that produces these tarballs
- [GitHub Actions: scheduled events](https://docs.github.com/en/actions/using-workflows/events-that-trigger-workflows#schedule)
- [GitHub Actions: encrypted secrets](https://docs.github.com/en/actions/security-guides/using-secrets-in-github-actions) — for `RBT_TARGET_SSH_KEY`
