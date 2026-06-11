#!/bin/sh
################################################################################
# POSIX S3 path routing for RBT packages (compatible with sh/dash in containers).
# Usage:
#   rbt-s3-upload-route.sh upload-deb
#   rbt-s3-upload-route.sh upload-rpm-tar
#   rbt-s3-upload-route.sh deb-repo-prefix
#   rbt-s3-upload-route.sh rpm-repo-prefix
################################################################################
set -eu

BASE="rbt"
EVENT="${GITHUB_EVENT_NAME:-}"
REF="${GITHUB_REF:-}"
REF_NAME="${GITHUB_REF_NAME:-}"
RUN_NUMBER="${GITHUB_RUN_NUMBER:-0}"
BUCKET="${AWS_S3_BUCKET:-}"
GITHUB_OUTPUT="${GITHUB_OUTPUT:-/dev/null}"

RBT_SKIP_UPLOAD="${RBT_SKIP_UPLOAD:-false}"
RBT_IS_DEFAULT_BRANCH="${RBT_IS_DEFAULT_BRANCH:-false}"
RBT_BRANCH_PREFIX="${RBT_BRANCH_PREFIX:-}"
RBT_BUILD_REF_NAME="${RBT_BUILD_REF_NAME:-}"

rbt_is_release_ref() {
  case "$1" in
    refs/heads/release/*) return 0 ;;
  esac
  return 1
}

rbt_resolve_route() {
  RBT_S3_ROUTE="pr"
  RBT_S3_DEB_PREFIX=""
  RBT_S3_RPM_PREFIX=""
  RBT_S3_TAR_PREFIX=""
  RBT_APT_SUITE="rbt-nightly"
  RBT_S3_OUTPUT_PATHS=""

  if [ "$RBT_SKIP_UPLOAD" = "true" ]; then
    RBT_S3_ROUTE="skip"
    return 0
  fi

  if [ "$EVENT" = "schedule" ] && [ "$RBT_IS_DEFAULT_BRANCH" != "true" ]; then
    RBT_S3_ROUTE="scheduled_branch"
    RBT_S3_DEB_PREFIX="${RBT_BRANCH_PREFIX}/${RBT_BUILD_REF_NAME}/nightly/deb"
    RBT_S3_RPM_PREFIX="${RBT_BRANCH_PREFIX}/${RBT_BUILD_REF_NAME}/nightly/rpm"
    RBT_S3_TAR_PREFIX="${RBT_BRANCH_PREFIX}/${RBT_BUILD_REF_NAME}/nightly/tar"
    RBT_S3_OUTPUT_PATHS="Ubuntu DEB|${RBT_BRANCH_PREFIX}/${RBT_BUILD_REF_NAME}/nightly/deb||CentOS/RHEL RPM|${RBT_BRANCH_PREFIX}/${RBT_BUILD_REF_NAME}/nightly/rpm||CentOS/RHEL TGZ|${RBT_BRANCH_PREFIX}/${RBT_BUILD_REF_NAME}/nightly/tar"
    return 0
  fi

  if rbt_is_release_ref "$REF" && { [ "$EVENT" = "push" ] || [ "$EVENT" = "workflow_dispatch" ]; }; then
    RBT_S3_ROUTE="release"
    RBT_S3_DEB_PREFIX="release/${BASE}/deb"
    RBT_S3_RPM_PREFIX="release/${BASE}/rpm"
    RBT_S3_TAR_PREFIX="release/${BASE}/tar"
    RBT_APT_SUITE="rbt-release"
    RBT_S3_OUTPUT_PATHS="Ubuntu DEB|release/${BASE}/deb||CentOS/RHEL RPM|release/${BASE}/rpm||CentOS/RHEL TGZ|release/${BASE}/tar"
    return 0
  fi

  if [ "$EVENT" = "schedule" ] || [ "$EVENT" = "push" ] || [ "$EVENT" = "workflow_dispatch" ]; then
    RBT_S3_ROUTE="nightly"
    RBT_S3_DEB_PREFIX="nightly/${BASE}/deb"
    RBT_S3_RPM_PREFIX="nightly/${BASE}/rpm"
    RBT_S3_TAR_PREFIX="nightly/${BASE}/tar"
    RBT_APT_SUITE="rbt-nightly"
    RBT_S3_OUTPUT_PATHS="Ubuntu DEB|nightly/${BASE}/deb||CentOS/RHEL RPM|nightly/${BASE}/rpm||CentOS/RHEL TGZ|nightly/${BASE}/tar"
    return 0
  fi

  RBT_S3_ROUTE="pr"
  RBT_S3_DEB_PREFIX="${BASE}/${REF_NAME}/${RUN_NUMBER}/deb"
  RBT_S3_RPM_PREFIX="${BASE}/${REF_NAME}/${RUN_NUMBER}/rpm"
  RBT_S3_TAR_PREFIX="${BASE}/${REF_NAME}/${RUN_NUMBER}/tar"
  RBT_S3_OUTPUT_PATHS="Ubuntu DEB|${RBT_S3_DEB_PREFIX}||CentOS/RHEL RPM|${RBT_S3_RPM_PREFIX}||TGZ|${RBT_S3_TAR_PREFIX}"
}

cmd="${1:-}"
rbt_resolve_route

case "$cmd" in
  upload-deb)
    if [ -z "$BUCKET" ]; then
      echo "::warning::AWS_S3_BUCKET not set. Skipping S3 upload."
      exit 0
    fi
    if [ "$RBT_S3_ROUTE" = "skip" ]; then
      echo "Skipping S3 upload (scheduled release* branch)."
      exit 0
    fi
    case "$RBT_S3_ROUTE" in
      scheduled_branch) echo "Scheduled ACTIVE_BRANCHES build: uploading DEB to ${RBT_S3_DEB_PREFIX}" ;;
      release)          echo "Release branch build: uploading DEB to ${RBT_S3_DEB_PREFIX}" ;;
      nightly)          echo "Nightly/push build: uploading DEB to ${RBT_S3_DEB_PREFIX}" ;;
      *)                echo "Uploading DEB to s3://${BUCKET}/${RBT_S3_DEB_PREFIX}/" ;;
    esac
    aws s3 cp ./build "s3://${BUCKET}/${RBT_S3_DEB_PREFIX}/" \
      --recursive --exclude "*" --include "amdrocm*-rbt*.deb" --no-progress
    echo "Listing s3://${BUCKET}/${RBT_S3_DEB_PREFIX}/"
    aws s3 ls "s3://${BUCKET}/${RBT_S3_DEB_PREFIX}/" --human-readable || true
    echo "bucket=${BUCKET}" >> "$GITHUB_OUTPUT"
    echo "paths=Ubuntu DEB|${RBT_S3_DEB_PREFIX}" >> "$GITHUB_OUTPUT"
    echo "Done."
    ;;
  upload-rpm-tar)
    if [ -z "$BUCKET" ]; then
      echo "::warning::AWS_S3_BUCKET not set. Skipping S3 upload."
      exit 0
    fi
    if [ "$RBT_S3_ROUTE" = "skip" ]; then
      echo "Skipping S3 upload (scheduled release* branch)."
      exit 0
    fi
    case "$RBT_S3_ROUTE" in
      scheduled_branch) echo "Scheduled ACTIVE_BRANCHES build: uploading to ${RBT_S3_RPM_PREFIX} and ${RBT_S3_TAR_PREFIX}" ;;
      release)          echo "Release branch build: uploading to ${RBT_S3_RPM_PREFIX} and ${RBT_S3_TAR_PREFIX}" ;;
      nightly)          echo "Nightly/push build: uploading to ${RBT_S3_RPM_PREFIX} and ${RBT_S3_TAR_PREFIX}" ;;
      *)                echo "Uploading to s3://${BUCKET}/${RBT_S3_RPM_PREFIX}/" ;;
    esac
    aws s3 cp ./build "s3://${BUCKET}/${RBT_S3_RPM_PREFIX}/" \
      --recursive --exclude "*" --include "amdrocm*-rbt*.rpm" --no-progress
    aws s3 cp ./build "s3://${BUCKET}/${RBT_S3_TAR_PREFIX}/" \
      --recursive --exclude "*" --include "amdrocm*-rbt*.tar.gz" --no-progress
    echo "Listing s3://${BUCKET}/${RBT_S3_RPM_PREFIX}/"
    aws s3 ls "s3://${BUCKET}/${RBT_S3_RPM_PREFIX}/" --human-readable || true
    echo "Listing s3://${BUCKET}/${RBT_S3_TAR_PREFIX}/"
    aws s3 ls "s3://${BUCKET}/${RBT_S3_TAR_PREFIX}/" --human-readable || true
    echo "bucket=${BUCKET}" >> "$GITHUB_OUTPUT"
    echo "paths=CentOS/RHEL RPM|${RBT_S3_RPM_PREFIX}||CentOS/RHEL TGZ|${RBT_S3_TAR_PREFIX}" >> "$GITHUB_OUTPUT"
    echo "Done."
    ;;
  deb-repo-prefix)
    echo "$RBT_S3_DEB_PREFIX"
    echo "$RBT_APT_SUITE"
    ;;
  rpm-repo-prefix)
    echo "$RBT_S3_RPM_PREFIX"
    ;;
  *)
    echo "Usage: $0 upload-deb|upload-rpm-tar|deb-repo-prefix|rpm-repo-prefix" >&2
    exit 1
    ;;
esac
