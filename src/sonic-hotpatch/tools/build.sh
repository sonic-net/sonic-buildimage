#!/bin/bash -e

INFO_FILE=$(./generate.py $@)

while [[ "$#" -gt 0 ]]; do
    case $1 in
        --patch-name)
            patch_name=$2; shift 2;;
        --extern-patch-name)
            extern_patch_name=$2; shift 2;;
        --patch-type)
            patch_type=$2; shift 2;;
        --sonic-version)
            sonic_version=$2; shift 2;;
        *) break ;;
    esac
done

# Check if running in a CI/non-interactive environment
# For Jenkins, you could use CI=true environment variable
if [ -z "$CI" ]; then
    "${EDITOR:-vim}" $INFO_FILE
fi
echo "Validating patch info..."
./check.py $INFO_FILE
if [ $? -eq 0 ]; then
    echo "Patch info validation passed."
else
    echo "Patch info validation failed!"
    exit 1
fi

cp -r src/* build/
cp -r packages/* build/

if [ -n "$patch_name" ]; then
    PATCH_PACKAGE=${patch_name%.tar.gz}
else
    PATCH_PACKAGE="SONiC-patch-$(date +%y%m%d%H%M%S)"
fi
rm -rf "${PATCH_PACKAGE}" && mv build "${PATCH_PACKAGE}"
tar -cvz --no-xattrs -C . -f "${PATCH_PACKAGE}.tar.gz" "${PATCH_PACKAGE}"

echo "=== Build completed successfully ==="
echo "Generated package: ${PATCH_PACKAGE}.tar.gz"
echo "Package contents:"
ls -lah "${PATCH_PACKAGE}.tar.gz"
