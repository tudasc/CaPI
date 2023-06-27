#!/bin/bash

scriptdir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

srcdir="$scriptdir/extern/src"
installdir="$scriptdir/extern/install"
moduledir="$scriptdir/extern/modulefiles"

mkdir -p "$srcdir"
mkdir -p "$installdir"
mkdir -p "$moduledir"

echo "Installing LLVM 13 with DSO support for XRay"
cd "$srcdir"

if [ ! -d "llvm-project-xray-dso" ]; then
  echo "[CaPI] Cloning LLVM repository..."
  git clone https://github.com/sebastiankreutzer/llvm-project-xray-dso.git -b "xray_dso" --single-branch --depth 1 llvm-project-xray-dso
else
  echo "[CaPI] LLVM is already downloaded! Re-building..."
fi

cd llvm-project-xray-dso
mkdir -p build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$installdir/llvm/13.0.1-xray-dso" -DLLVM_ENABLE_PROJECTS="clang;compiler-rt;libcxx;libcxxabi;openmp" -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_INSTALL_UTILS=ON ../llvm/
ninja
ninja install

# Creating LLVM modulefile
llvm_module_dir="$moduledir/llvm/"
mkdir -p "$llvm_module_dir"
cat << EOF > "$llvm_module_dir/13.0.1-xray-dso"
#%Module1.0
proc ModulesHelp { } {
global dotversion
puts stderr "\tLLVM 13.0.1 with XRay DSO instrumentation"
}

set pkg_dir="$installdir/llvm/13.0.1-xray-dso"

module-whatis "LLVM 13.0.1 with XRay DSO instrumentation"

conflict llvm

prepend-path PATH $pkg_dir/bin
prepend-path LD_LIBRARY_PATH $pkg_dir/lib
prepend-path LIBRARY_PATH $pkg_dir/lib
prepend-path MANPATH $pkg_dir/share/man
EOF
echo "[CaPI] Generated modulefile for LLVM. Run 'module use $llvm_module_dir' to activate."

