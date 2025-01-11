{ pkgs ? import <nixpkgs> {} , withEditor ? false }:

pkgs.stdenv.mkDerivation {
  name = "openclonk";

  gitRef = pkgs.lib.commitIdFromGitRepo ../.git;

  src = builtins.filterSource (path: type: ! builtins.elem (baseNameOf path) [
    ".git" # leave out .git as it changes often in ways that do not affect the build
    "default.nix" # default.nix might change, but the only thing that matters is what it evaluates to, and nix takes care of that
    "result" # build result is irrelevant
    "build"
  ]) ./..;

  enableParallelInstalling = false;

  nativeBuildInputs = with pkgs; [ cmake pkg-config ];

  dontStrip = true;

  buildInputs = with pkgs; [
    SDL2
    libvorbis
    libogg
    libjpeg
    libpng
    freetype
    glew
    tinyxml
    openal
    freealut
    libepoxy
    curl
    readline
    miniupnpc
  ] ++ pkgs.lib.optional withEditor qt5.full;

  cmakeFlags = [ "-DCMAKE_AR=${pkgs.gcc-unwrapped}/bin/gcc-ar" "-DCMAKE_RANLIB=${pkgs.gcc-unwrapped}/bin/gcc-ranlib" ];

  preConfigure = ''
    sed s/REVGOESHERE/''${gitRef:0:12}/ > cmake/GitGetChangesetID.cmake <<EOF
    function(git_get_changeset_id VAR)
      set(\''${VAR} "REVGOESHERE" PARENT_SCOPE)
    endfunction()
    EOF
  '';

  cmakeBuildType = "RelWithDebInfo";

  postInstall = ''
    mv -v $out/games/openclonk $out/bin/
  '';

  meta = with pkgs.lib; {
    description = "A free multiplayer action game about mining, settling and fast-paced melees";
    homepage = "http://www.openclonk.org/";
    license = with licenses; [
      isc cc-by-30
    ];
  };
}
