{ pkgs ? import <nixpkgs> { } }:
with pkgs;
let
  libsocketcan = stdenv.mkDerivation rec {
    pname = "libsocketcan";
    rev = "b464485";
    commits = "101";
    version = "r${commits}.${rev}";

    src = fetchgit {
      url = "https://github.com/lalten/libsocketcan";
      inherit rev;
      sha256 = "sha256-MgHdiVju327plJnSBABA2ugzLZETQpPvUg7JluaIJQU=";
    };

    nativeBuildInputs = [ autoreconfHook ];

    preConfigure = ''
      patchShebangs .
      ./autogen.sh
      ./configure --prefix=$out
    '';

    preBuild = ''
      cp README.md README
      ls
    '';
  };
in
mkShell rec {
  nativeBuildInputs = [
    pkg-config
    clang-tools
  ];

  buildInputs = [
    python3
    zeromq
    libsocketcan
  ];

  inputsFrom = [ python3 ];

  PROJECT_ROOT = builtins.getEnv "PWD";

  shellHook = ''
    export LD_LIBRARY_PATH=${PROJECT_ROOT}/build:$LD_LIBRARY_PATH
    export PYTHONPATH=${PROJECT_ROOT}/build:$PYTHONPATH
  '';
}
