{ pkgs ? import <nixpkgs> {} , withEditor ? false }:

pkgs.stdenv.mkDerivation rec {
  name = "openclonk";

  gitRef = pkgs.lib.commitIdFromGitRepo ../.git;

  src = builtins.filterSource (path: type: ! builtins.elem (baseNameOf path) [
    ".git" # leave out .git as it changes often in ways that do not affect the build
    "default.nix" # default.nix might change, but the only thing that matters is what it evaluates to, and nix takes care of that
    "result" # build result is irrelevant
    "build"
  ]) ./..;

  enableParallelBuilding = true;

  hardeningDisable = [ "format" ];

  nativeBuildInputs = with pkgs; [ cmake pkgconfig ];

  dontStrip = true;

  buildInputs = with pkgs; [
    SDL2 libvorbis libogg libjpeg libpng freetype epoxy tinyxml
    openal freealut readline
  ] ++ stdenv.lib.optional withEditor qt5.full;

  cmakeFlags = [ "-DCMAKE_AR=${pkgs.gcc-unwrapped}/bin/gcc-ar" "-DCMAKE_RANLIB=${pkgs.gcc-unwrapped}/bin/gcc-ranlib" ];

  preConfigure = ''
    sed s/REVGOESHERE/''${gitRef:0:12}/ > cmake/GitGetChangesetID.cmake <<EOF
    function(git_get_changeset_id VAR)
      set(\''${VAR} "REVGOESHERE" PARENT_SCOPE)
    endfunction()
    EOF
  '';

  # Temporary measure until 28154f45a6 is in a release of nixpkgs
  # Once this is the case, replace this with cmakeBuildType = "RelWithDebInfo"
  configurePhase = ''
    runHook preConfigure
    fixCmakeFiles .
    mkdir build
    cd build
    cmakeDir=..
    cmakeFlagsArray+=("-DCMAKE_INSTALL_PREFIX=$prefix" "-DCMAKE_BUILD_TYPE=RelWithDebInfo" "-DCMAKE_SKIP_BUILD_RPATH=ON")
    cmake ''${cmakeDir:-.} $cmakeFlags "''${cmakeFlagsArray[@]}"
    runHook postConfigure
  '';

  postInstall = ''
    mkdir -p $out/bin
  '';


  meta = with pkgs.stdenv.lib; {
    description = "A free multiplayer action game about mining, settling and fast-paced melees";
    homepage = "http://www.openclonk.org/";
    license = with licenses; [
      isc cc-by-30
    ];
    maintainers = with lib.maintainers; [ lheckemann ];
  };
}
