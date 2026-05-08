#!/bin/bash
################################################################################
# Remote RBT nightly install + test driver (used by rbt-nightly-tests.yml).
# Mirrors build_packages_local.sh: workflow orchestrates, this script executes.
################################################################################

set -euo pipefail

usage() {
  cat <<'EOF'
Usage: rbt_nightly_test.sh <command>

Commands (workflow order):
  validate-config     Fail fast; emit prepare job outputs to GITHUB_OUTPUT
  setup-ssh           Write SSH key/config under RUNNER_TEMP and verify connectivity
  verify-rocm         Remote rocminfo + amd-smi on TARGET_ROCM_PATH
  download-tarball    curl tarball to ./pkg/ on the orchestrator runner
  copy-to-target      scp tarball to REMOTE_WORK_DIR/pkg on target
  install-rbt         Extract tarball on target under INSTALL_DIR
  verify-rbt-binary   Remote ldd check on RBT_BIN
  run-tests           Run healthcheck, p2p, topology; write rc/start/end to GITHUB_OUTPUT
  collect-logs        scp remote logs to ./reports/
  capture-versions    Write rbt_version and target_rocm_version to GITHUB_OUTPUT
  build-report        Write ./reports/SUMMARY.md from env + prior outputs
  cleanup-remote      rm -rf REMOTE_WORK_DIR on target
  cleanup-local-ssh   Remove workflow-scoped key/config files
EOF
}

require_env() {
  local name="$1"
  if [ -z "${!name:-}" ]; then
    echo "::error::Required environment variable $name is not set" >&2
    exit 1
  fi
}

ld_path_export() {
  echo "${INSTALL_DIR}/lib:${TARGET_ROCM_PATH}/lib/rocm_sysdeps/lib:${TARGET_ROCM_PATH}/lib/llvm/lib:${TARGET_ROCM_PATH}/lib"
}

cmd_validate_config() {
  require_env TARBALL_NAME
  require_env TARGET_NODE
  require_env TARGET_ROCM_PATH

  local input_remote="${INPUT_REMOTE_WORK_DIR:-}"
  local var_remote="${VAR_REMOTE_WORK_DIR:-}"
  local run_id="${GITHUB_RUN_ID:-local}"

  if [ -n "$input_remote" ]; then
    REMOTE_WORK_DIR="$input_remote"
  elif [ -n "$var_remote" ]; then
    REMOTE_WORK_DIR="$var_remote"
  else
    REMOTE_WORK_DIR="/tmp/rbt-nightly-${run_id}"
  fi

  if [[ "$TARBALL_NAME" =~ ^amdrocm([0-9]+)- ]]; then
    ROCM_MAJOR="${BASH_REMATCH[1]}"
  else
    echo "::error::Cannot parse ROCm major version from tarball name: $TARBALL_NAME" >&2
    exit 1
  fi

  INSTALL_DIR="/opt/rocm/extras-${ROCM_MAJOR}"
  RBT_BIN="${INSTALL_DIR}/bin/rocm_bandwidth_test"

  echo "Orchestrator runner : $(hostname)"
  echo "Target node         : ${TARGET_USER:-}@${TARGET_NODE}"
  echo "Target ROCm path    : $TARGET_ROCM_PATH"
  echo "Remote work dir     : $REMOTE_WORK_DIR"
  echo "ROCm major          : $ROCM_MAJOR"
  echo "Expected RBT binary : $RBT_BIN"

  if [ -n "${GITHUB_OUTPUT:-}" ]; then
    {
      echo "remote_work_dir=$REMOTE_WORK_DIR"
      echo "rocm_major=$ROCM_MAJOR"
      echo "install_dir=$INSTALL_DIR"
      echo "rbt_bin=$RBT_BIN"
    } >> "$GITHUB_OUTPUT"
  fi

  export REMOTE_WORK_DIR ROCM_MAJOR INSTALL_DIR RBT_BIN
}

ssh_state_paths() {
  local base="${RUNNER_TEMP:-/tmp}"
  SSH_KEY_FILE="${base}/rbt_target_key"
  SSH_CONFIG_FILE="${base}/rbt_target_ssh_config"
  mkdir -p "$base"
}

cmd_setup_ssh() {
  require_env TARGET_NODE
  require_env REMOTE_WORK_DIR

  ssh_state_paths

  if [ -n "${GITHUB_ENV:-}" ]; then
    {
      echo "SSH_KEY_FILE=$SSH_KEY_FILE"
      echo "SSH_CONFIG_FILE=$SSH_CONFIG_FILE"
    } >> "$GITHUB_ENV"
  fi

  if [ -z "${SSH_PRIVATE_KEY:-}" ]; then
    echo "::error::SSH_PRIVATE_KEY is not set; cannot SSH to target node." >&2
    exit 1
  fi

  install -m 600 /dev/null "$SSH_KEY_FILE"
  printf '%s\n' "$SSH_PRIVATE_KEY" > "$SSH_KEY_FILE"
  chmod 600 "$SSH_KEY_FILE"

  local known_hosts="${SSH_CONFIG_FILE}.known_hosts"
  : > "$known_hosts"
  chmod 600 "$known_hosts"
  ssh-keyscan -T 15 -H "$TARGET_NODE" >> "$known_hosts" 2>/dev/null || true

  cat > "$SSH_CONFIG_FILE" <<EOF
Host rbt-target
  HostName $TARGET_NODE
  User ${TARGET_USER:-}
  IdentityFile $SSH_KEY_FILE
  IdentitiesOnly yes
  BatchMode yes
  LogLevel ERROR
  StrictHostKeyChecking accept-new
  UserKnownHostsFile $known_hosts
  ServerAliveInterval 30
  ServerAliveCountMax 10
EOF
  chmod 600 "$SSH_CONFIG_FILE"

  echo "Verifying SSH connectivity to ${TARGET_USER:-}@${TARGET_NODE}..."
  if ! ssh -F "$SSH_CONFIG_FILE" rbt-target 'echo __START__; hostname; id; uptime' \
    2>&1 | sed -n '/^__START__$/,$p' | tail -n +2; then
    echo "::error::SSH connectivity check failed for ${TARGET_USER:-}@${TARGET_NODE}. Verify secrets RBT_TARGET_NODE, RBT_TARGET_USER, RBT_TARGET_SSH_KEY and network access from this runner." >&2
    exit 1
  fi

  ssh -q -F "$SSH_CONFIG_FILE" rbt-target \
    "mkdir -p '${REMOTE_WORK_DIR}/pkg' '${REMOTE_WORK_DIR}/reports'"
}

cmd_verify_rocm() {
  require_env SSH_CONFIG_FILE
  require_env TARGET_ROCM_PATH

  ssh -q -F "$SSH_CONFIG_FILE" rbt-target \
    "TARGET_ROCM_PATH='$TARGET_ROCM_PATH' bash -s" <<'REMOTE'
set -euo pipefail
echo "=== System ==="
uname -a
echo
echo "=== Target ROCm path: ${TARGET_ROCM_PATH} ==="
if [ ! -d "${TARGET_ROCM_PATH}" ]; then
  echo "::error::${TARGET_ROCM_PATH} does not exist on the target node."
  exit 1
fi
echo "=== ${TARGET_ROCM_PATH}/bin/rocminfo ==="
"${TARGET_ROCM_PATH}/bin/rocminfo"
echo
echo "=== ${TARGET_ROCM_PATH}/bin/amd-smi version ==="
"${TARGET_ROCM_PATH}/bin/amd-smi" version || echo "amd-smi not available"
echo
echo "::notice::ROCm prerequisites OK on target node at ${TARGET_ROCM_PATH}"
REMOTE
}

cmd_download_tarball() {
  require_env TARBALL_URL
  require_env TARBALL_NAME
  mkdir -p ./pkg
  echo "Downloading: $TARBALL_URL"
  curl -fL --max-time 600 -o "./pkg/${TARBALL_NAME}" "${TARBALL_URL}"
  ls -la ./pkg/
  file "./pkg/${TARBALL_NAME}" || true
}

cmd_copy_to_target() {
  require_env SSH_CONFIG_FILE
  require_env TARBALL_NAME
  require_env REMOTE_WORK_DIR
  echo "Copying ./pkg/${TARBALL_NAME} -> rbt-target:${REMOTE_WORK_DIR}/pkg/"
  scp -q -F "$SSH_CONFIG_FILE" "./pkg/${TARBALL_NAME}" \
    "rbt-target:${REMOTE_WORK_DIR}/pkg/${TARBALL_NAME}"
  ssh -q -F "$SSH_CONFIG_FILE" rbt-target "ls -la '${REMOTE_WORK_DIR}/pkg/'"
}

cmd_install_rbt() {
  require_env SSH_CONFIG_FILE
  require_env TARBALL_NAME
  require_env REMOTE_WORK_DIR
  require_env ROCM_MAJOR
  require_env INSTALL_DIR
  require_env RBT_BIN
  require_env TARGET_ROCM_PATH

  ssh -q -F "$SSH_CONFIG_FILE" rbt-target \
    "TARBALL_NAME='$TARBALL_NAME' REMOTE_WORK_DIR='$REMOTE_WORK_DIR' ROCM_MAJOR='$ROCM_MAJOR' INSTALL_DIR='$INSTALL_DIR' RBT_BIN='$RBT_BIN' TARGET_ROCM_PATH='$TARGET_ROCM_PATH' bash -s" <<'REMOTE'
set -euo pipefail
PKG="${REMOTE_WORK_DIR}/pkg/${TARBALL_NAME}"
echo "Target ROCm path            : ${TARGET_ROCM_PATH}"
echo "Detected ROCm major version : ${ROCM_MAJOR}"
echo "Install target              : ${INSTALL_DIR}"
echo "Expected RBT binary         : ${RBT_BIN}"
if [ ! -f "$PKG" ]; then
  echo "::error::Tarball not found on target node at $PKG"
  exit 1
fi
probe="$INSTALL_DIR"
while [ ! -d "$probe" ] && [ "$probe" != "/" ]; do probe=$(dirname "$probe"); done
if [ -w "$probe" ]; then
  echo "Installing into $INSTALL_DIR without sudo (writable path)"
  mkdir -p "$INSTALL_DIR"
  tar -xzf "$PKG" -C "$INSTALL_DIR"
else
  echo "Installing into $INSTALL_DIR via sudo -n (path not user-writable)"
  sudo -n mkdir -p "$INSTALL_DIR"
  sudo -n tar -xzf "$PKG" -C "$INSTALL_DIR"
fi
if [ ! -x "$RBT_BIN" ]; then
  echo "::error::rocm_bandwidth_test binary not found or not executable at $RBT_BIN after install"
  ls -la "$INSTALL_DIR/" || true
  ls -la "$INSTALL_DIR/bin/" || true
  exit 1
fi
export LD_LIBRARY_PATH="${INSTALL_DIR}/lib:${TARGET_ROCM_PATH}/lib/rocm_sysdeps/lib:${TARGET_ROCM_PATH}/lib/llvm/lib:${TARGET_ROCM_PATH}/lib:${LD_LIBRARY_PATH:-}"
echo "Installed RBT at: $RBT_BIN"
"$RBT_BIN" --version || true
REMOTE
}

cmd_verify_rbt_binary() {
  require_env SSH_CONFIG_FILE
  require_env RBT_BIN
  require_env INSTALL_DIR
  require_env TARGET_ROCM_PATH

  ssh -q -F "$SSH_CONFIG_FILE" rbt-target \
    "RBT_BIN='$RBT_BIN' INSTALL_DIR='$INSTALL_DIR' TARGET_ROCM_PATH='$TARGET_ROCM_PATH' bash -s" <<'REMOTE'
set -euo pipefail
if [ ! -x "$RBT_BIN" ]; then
  echo "::error::$RBT_BIN was not produced by tarball extraction on target."
  exit 1
fi
export LD_LIBRARY_PATH="${INSTALL_DIR}/lib:${TARGET_ROCM_PATH}/lib/rocm_sysdeps/lib:${TARGET_ROCM_PATH}/lib/llvm/lib:${TARGET_ROCM_PATH}/lib:${LD_LIBRARY_PATH:-}"
echo "=== ldd $RBT_BIN ==="
LDD_OUTPUT=$(ldd "$RBT_BIN" 2>&1 || true)
echo "$LDD_OUTPUT"
if echo "$LDD_OUTPUT" | grep -q "not found"; then
  echo "::error::RBT binary has unresolved library dependencies on target (see above)."
  exit 1
fi
echo "::notice::RBT binary's library dependencies resolved OK on target."
REMOTE
}

cmd_run_tests() {
  require_env SSH_CONFIG_FILE
  require_env RBT_BIN
  require_env INSTALL_DIR
  require_env REMOTE_WORK_DIR
  require_env TARGET_ROCM_PATH
  require_env TARGET_NODE

  set +e
  local rc_hc rc_p2p rc_topo
  local start_hc end_hc start_p2p end_p2p start_topo end_topo

  # ── Healthcheck ──────────────────────────────────────────────────────────
  start_hc=$(date -u +%FT%TZ)
  echo "::group::TransferBench healthcheck (${RBT_BIN} run transferbench healthcheck) on ${TARGET_NODE}"
  ssh -q -F "$SSH_CONFIG_FILE" rbt-target \
    "RBT_BIN='$RBT_BIN' INSTALL_DIR='$INSTALL_DIR' REMOTE_WORK_DIR='$REMOTE_WORK_DIR' TARGET_ROCM_PATH='$TARGET_ROCM_PATH' bash -s" <<'REMOTE'
set +e
export LD_LIBRARY_PATH="${INSTALL_DIR}/lib:${TARGET_ROCM_PATH}/lib/rocm_sysdeps/lib:${TARGET_ROCM_PATH}/lib/llvm/lib:${TARGET_ROCM_PATH}/lib:${LD_LIBRARY_PATH:-}"
mkdir -p "${REMOTE_WORK_DIR}/reports"
"$RBT_BIN" run transferbench healthcheck --verbose 2>&1 | tee "${REMOTE_WORK_DIR}/reports/healthcheck.log"
RC=${PIPESTATUS[0]}
echo "remote_rc=$RC"
exit $RC
REMOTE
  rc_hc=$?
  echo "::endgroup::"
  end_hc=$(date -u +%FT%TZ)

  # ── P2P bandwidth ────────────────────────────────────────────────────────
  start_p2p=$(date -u +%FT%TZ)
  echo "::group::TransferBench P2P bandwidth (${RBT_BIN} run transferbench p2p) on ${TARGET_NODE}"
  ssh -q -F "$SSH_CONFIG_FILE" rbt-target \
    "RBT_BIN='$RBT_BIN' INSTALL_DIR='$INSTALL_DIR' REMOTE_WORK_DIR='$REMOTE_WORK_DIR' TARGET_ROCM_PATH='$TARGET_ROCM_PATH' bash -s" <<'REMOTE'
set +e
export LD_LIBRARY_PATH="${INSTALL_DIR}/lib:${TARGET_ROCM_PATH}/lib/rocm_sysdeps/lib:${TARGET_ROCM_PATH}/lib/llvm/lib:${TARGET_ROCM_PATH}/lib:${LD_LIBRARY_PATH:-}"
mkdir -p "${REMOTE_WORK_DIR}/reports"
"$RBT_BIN" run transferbench p2p --size=256MB --iterations=10 2>&1 | tee "${REMOTE_WORK_DIR}/reports/p2p.log"
RC=${PIPESTATUS[0]}
echo "remote_rc=$RC"
exit $RC
REMOTE
  rc_p2p=$?
  echo "::endgroup::"
  end_p2p=$(date -u +%FT%TZ)

  # ── Topology ─────────────────────────────────────────────────────────────
  start_topo=$(date -u +%FT%TZ)
  echo "::group::TransferBench topology (${RBT_BIN} run transferbench topology) on ${TARGET_NODE}"
  ssh -q -F "$SSH_CONFIG_FILE" rbt-target \
    "RBT_BIN='$RBT_BIN' INSTALL_DIR='$INSTALL_DIR' REMOTE_WORK_DIR='$REMOTE_WORK_DIR' TARGET_ROCM_PATH='$TARGET_ROCM_PATH' bash -s" <<'REMOTE'
set +e
export LD_LIBRARY_PATH="${INSTALL_DIR}/lib:${TARGET_ROCM_PATH}/lib/rocm_sysdeps/lib:${TARGET_ROCM_PATH}/lib/llvm/lib:${TARGET_ROCM_PATH}/lib:${LD_LIBRARY_PATH:-}"
mkdir -p "${REMOTE_WORK_DIR}/reports"
"$RBT_BIN" run transferbench topology 2>&1 | tee "${REMOTE_WORK_DIR}/reports/topology.log"
RC=${PIPESTATUS[0]}
echo "remote_rc=$RC"
exit $RC
REMOTE
  rc_topo=$?
  echo "::endgroup::"
  end_topo=$(date -u +%FT%TZ)

  if [ -n "${GITHUB_OUTPUT:-}" ]; then
    {
      echo "rc_healthcheck=$rc_hc"
      echo "start_healthcheck=$start_hc"
      echo "end_healthcheck=$end_hc"
      echo "rc_p2p=$rc_p2p"
      echo "start_p2p=$start_p2p"
      echo "end_p2p=$end_p2p"
      echo "rc_topology=$rc_topo"
      echo "start_topology=$start_topo"
      echo "end_topology=$end_topo"
    } >> "$GITHUB_OUTPUT"
  fi
  # Caller workflow decides whether to fail the job on non-zero rc.
  return 0
}

cmd_collect_logs() {
  require_env SSH_CONFIG_FILE
  require_env REMOTE_WORK_DIR
  mkdir -p ./reports
  if [ ! -f "$SSH_CONFIG_FILE" ]; then
    echo "::warning::SSH config not present; skipping log collection."
    return 0
  fi
  echo "Copying logs from rbt-target:${REMOTE_WORK_DIR}/reports/ -> ./reports/"
  scp -q -F "$SSH_CONFIG_FILE" "rbt-target:${REMOTE_WORK_DIR}/reports/*.log" ./reports/ || \
    echo "::warning::No log files retrieved from target node."
  ls -la ./reports/ || true
}

cmd_capture_versions() {
  require_env SSH_CONFIG_FILE
  require_env RBT_BIN
  require_env INSTALL_DIR
  require_env TARGET_ROCM_PATH

  local rbt_version target_rocm_version
  rbt_version=$(
    ssh -q -F "$SSH_CONFIG_FILE" rbt-target \
      "RBT_BIN='$RBT_BIN' INSTALL_DIR='$INSTALL_DIR' TARGET_ROCM_PATH='$TARGET_ROCM_PATH' bash -s" <<'REMOTE' 2>/dev/null | head -1 || echo "unknown"
export LD_LIBRARY_PATH="${INSTALL_DIR}/lib:${TARGET_ROCM_PATH}/lib/rocm_sysdeps/lib:${TARGET_ROCM_PATH}/lib/llvm/lib:${TARGET_ROCM_PATH}/lib:${LD_LIBRARY_PATH:-}"
"$RBT_BIN" --version 2>/dev/null
REMOTE
  )
  target_rocm_version=$(
    ssh -q -F "$SSH_CONFIG_FILE" rbt-target \
      "TARGET_ROCM_PATH='$TARGET_ROCM_PATH' bash -s" <<'REMOTE' 2>/dev/null || echo "unknown"
cat "${TARGET_ROCM_PATH}/.info/version" 2>/dev/null \
  || cat "${TARGET_ROCM_PATH}/share/doc/rocm-core/version" 2>/dev/null \
  || echo "unknown"
REMOTE
  )

  if [ -n "${GITHUB_OUTPUT:-}" ]; then
    echo "rbt_version=$rbt_version" >> "$GITHUB_OUTPUT"
    echo "target_rocm_version=$target_rocm_version" >> "$GITHUB_OUTPUT"
  fi
}

cmd_build_report() {
  require_env TARBALL_NAME
  require_env TARBALL_URL
  require_env TARGET_ROCM_PATH
  require_env REMOTE_WORK_DIR
  require_env RBT_BIN

  local rc_hc="${RBT_HEALTHCHECK_RC:-0}"
  local rc_p2p="${RBT_P2P_RC:-0}"
  local rc_topo="${RBT_TOPOLOGY_RC:-0}"
  local start_hc="${RBT_HEALTHCHECK_START:-}"
  local end_hc="${RBT_HEALTHCHECK_END:-}"
  local start_p2p="${RBT_P2P_START:-}"
  local end_p2p="${RBT_P2P_END:-}"
  local start_topo="${RBT_TOPOLOGY_START:-}"
  local end_topo="${RBT_TOPOLOGY_END:-}"
  local rbt_version="${RBT_VERSION:-unknown}"
  local target_rocm_version="${TARGET_ROCM_VERSION:-unknown}"
  local run_id="${GITHUB_RUN_ID:-local}"
  local server_url="${GITHUB_SERVER_URL:-https://github.com}"
  local repository="${GITHUB_REPOSITORY:-local/repo}"
  local event_name="${GITHUB_EVENT_NAME:-local}"

  mkdir -p ./reports

  local s_hc s_p2p s_topo overall
  [ "$rc_hc"   -eq 0 ] && s_hc=PASS   || s_hc=FAIL
  [ "$rc_p2p"  -eq 0 ] && s_p2p=PASS  || s_p2p=FAIL
  [ "$rc_topo" -eq 0 ] && s_topo=PASS || s_topo=FAIL

  if [ "$rc_hc" -eq 0 ] && [ "$rc_p2p" -eq 0 ] && [ "$rc_topo" -eq 0 ]; then
    overall=PASS
  else
    overall=FAIL
  fi

  local report=./reports/SUMMARY.md
  {
    echo "# RBT Nightly Test Report"
    echo ""
    echo "| Field | Value |"
    echo "|---|---|"
    echo "| Run | [\`${run_id}\`](${server_url}/${repository}/actions/runs/${run_id}) |"
    echo "| Trigger | \`${event_name}\` |"
    echo "| Target ROCm path | \`${TARGET_ROCM_PATH}\` (version \`${target_rocm_version}\`) |"
    echo "| Remote work dir | \`${REMOTE_WORK_DIR}\` |"
    echo "| Tarball | \`${TARBALL_NAME}\` |"
    echo "| Source URL | ${TARBALL_URL} |"
    echo "| RBT version | \`${rbt_version}\` |"
    echo "| Overall result | **${overall}** |"
    echo ""
    echo "## Results"
    echo ""
    echo "| Test | Command | Result | Exit | Started (UTC) | Ended (UTC) |"
    echo "|---|---|:--:|---:|---|---|"
    echo "| Healthcheck | \`${RBT_BIN} run transferbench healthcheck\` | ${s_hc} | ${rc_hc} | ${start_hc} | ${end_hc} |"
    echo "| P2P Bandwidth | \`${RBT_BIN} run transferbench p2p --size=256MB --iterations=10\` | ${s_p2p} | ${rc_p2p} | ${start_p2p} | ${end_p2p} |"
    echo "| Topology | \`${RBT_BIN} run transferbench topology\` | ${s_topo} | ${rc_topo} | ${start_topo} | ${end_topo} |"
    echo ""
    echo "## Logs"
    echo ""
    echo "Full stdout/stderr from test runs is attached to the artifact"
    echo "\`rbt-nightly-report-${run_id}\`:"
    echo ""
    echo "- \`healthcheck.log\`"
    echo "- \`p2p.log\`"
    echo "- \`topology.log\`"
    echo "- \`SUMMARY.md\` (this file)"
  } > "$report"

  cat "$report"
  if [ -n "${GITHUB_STEP_SUMMARY:-}" ]; then
    cat "$report" >> "$GITHUB_STEP_SUMMARY"
  fi

  if [ -n "${GITHUB_OUTPUT:-}" ]; then
    echo "overall=$overall" >> "$GITHUB_OUTPUT"
  fi
}

cmd_cleanup_remote() {
  set +e
  if [ -z "${REMOTE_WORK_DIR:-}" ] || [ -z "${SSH_CONFIG_FILE:-}" ] || [ ! -f "$SSH_CONFIG_FILE" ]; then
    return 0
  fi
  ssh -q -F "$SSH_CONFIG_FILE" rbt-target "rm -rf '${REMOTE_WORK_DIR}'" || \
    echo "::warning::Failed to clean up ${REMOTE_WORK_DIR} on target node."
}

cmd_cleanup_local_ssh() {
  set +e
  if [ -n "${SSH_KEY_FILE:-}" ]; then
    rm -f "$SSH_KEY_FILE"
  fi
  if [ -n "${SSH_CONFIG_FILE:-}" ]; then
    rm -f "$SSH_CONFIG_FILE" "${SSH_CONFIG_FILE}.known_hosts"
  fi
}

main() {
  if [ $# -lt 1 ]; then
    usage
    exit 1
  fi
  case "$1" in
    validate-config)    cmd_validate_config ;;
    setup-ssh)          cmd_setup_ssh ;;
    verify-rocm)        cmd_verify_rocm ;;
    download-tarball)   cmd_download_tarball ;;
    copy-to-target)     cmd_copy_to_target ;;
    install-rbt)        cmd_install_rbt ;;
    verify-rbt-binary)  cmd_verify_rbt_binary ;;
    run-tests)          cmd_run_tests ;;
    collect-logs)       cmd_collect_logs ;;
    capture-versions)   cmd_capture_versions ;;
    build-report)       cmd_build_report ;;
    cleanup-remote)     cmd_cleanup_remote ;;
    cleanup-local-ssh)  cmd_cleanup_local_ssh ;;
    -h|--help)          usage ;;
    *)
      echo "::error::Unknown command: $1" >&2
      usage
      exit 1
      ;;
  esac
}

main "$@"
