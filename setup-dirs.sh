cfg="Debug"
build_dir_prefix="./build"
build_dir="$build_dir_prefix/debug"
source_dir_from_build_dir="../../src"

if [ "$1" == "release" ];
then
    cfg="Release"
    build_dir="$build_dir_prefix/release"
fi
