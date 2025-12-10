#!/bin/bash
# Build and push multi-architecture Docker image for X-CASH Migration Network
#
# This script builds Docker images for both amd64 and arm64 architectures
# and pushes them to Docker Hub as xcashtech/xcash-core-migration
#
# Prerequisites:
#   - Docker buildx configured with multi-arch support
#   - Docker Hub authentication (docker login)
#   - Sufficient disk space for multi-arch builds
#
# Usage:
#   ./build-and-push.sh [--no-cache] [--tag TAG]

set -e  # Exit on error

# Default configuration
IMAGE_NAME="xcashtech/xcash-core-migration"
IMAGE_TAG="latest"
USE_CACHE="--cache-from=type=registry,ref=${IMAGE_NAME}:buildcache"
PLATFORMS="linux/amd64,linux/arm64"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    --no-cache)
      USE_CACHE="--no-cache"
      echo "⚠️  Cache disabled"
      shift
      ;;
    --tag)
      IMAGE_TAG="$2"
      echo "📦 Using custom tag: $IMAGE_TAG"
      shift 2
      ;;
    --platform)
      PLATFORMS="$2"
      echo "🏗️  Building for platforms: $PLATFORMS"
      shift 2
      ;;
    --help)
      echo "Usage: $0 [OPTIONS]"
      echo ""
      echo "Options:"
      echo "  --no-cache       Build without using cache"
      echo "  --tag TAG        Use custom tag (default: latest)"
      echo "  --platform LIST  Comma-separated platform list (default: linux/amd64,linux/arm64)"
      echo "  --help           Show this help message"
      exit 0
      ;;
    *)
      echo "❌ Unknown option: $1"
      echo "Use --help for usage information"
      exit 1
      ;;
  esac
done

# Verify we're in the correct directory
if [ ! -f "Dockerfile" ]; then
  echo "❌ Error: Dockerfile not found"
  echo "Please run this script from the xcash-core-migration root directory"
  exit 1
fi

# Check Docker buildx is available
if ! docker buildx version > /dev/null 2>&1; then
  echo "❌ Error: docker buildx not found"
  echo "Please install Docker with buildx support"
  exit 1
fi

# Ensure buildx builder exists
if ! docker buildx inspect xcash-builder > /dev/null 2>&1; then
  echo "🔧 Creating buildx builder instance..."
  docker buildx create --name xcash-builder --use --bootstrap
else
  echo "✅ Using existing buildx builder: xcash-builder"
  docker buildx use xcash-builder
fi

# Get git commit info for build metadata
GIT_COMMIT=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
BUILD_DATE=$(date -u +'%Y-%m-%dT%H:%M:%SZ')

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Building X-CASH Migration Network Docker Image               ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "📦 Image:        $IMAGE_NAME:$IMAGE_TAG"
echo "🏗️  Platforms:    $PLATFORMS"
echo "🔨 Git commit:   $GIT_COMMIT ($GIT_BRANCH)"
echo "📅 Build date:   $BUILD_DATE"
echo "💾 Cache:        $([ "$USE_CACHE" = "--no-cache" ] && echo "disabled" || echo "enabled")"
echo ""

# Confirm before proceeding
read -p "🤔 Proceed with build and push? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
  echo "❌ Build cancelled"
  exit 1
fi

# Build and push
echo ""
echo "🚀 Starting multi-architecture build..."
echo ""

docker buildx build \
  $USE_CACHE \
  --platform "$PLATFORMS" \
  --build-arg "GIT_COMMIT=$GIT_COMMIT" \
  --build-arg "GIT_BRANCH=$GIT_BRANCH" \
  --build-arg "BUILD_DATE=$BUILD_DATE" \
  -t "${IMAGE_NAME}:${IMAGE_TAG}" \
  -t "${IMAGE_NAME}:${GIT_COMMIT}" \
  --push \
  --progress=plain \
  .

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  ✅ Build Complete!                                            ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
echo "📦 Images pushed:"
echo "   - ${IMAGE_NAME}:${IMAGE_TAG}"
echo "   - ${IMAGE_NAME}:${GIT_COMMIT}"
echo ""
echo "🔍 View on Docker Hub:"
echo "   https://hub.docker.com/r/${IMAGE_NAME}/tags"
echo ""
echo "📥 Pull command:"
echo "   docker pull ${IMAGE_NAME}:${IMAGE_TAG}"
echo ""
