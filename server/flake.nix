{
	inputs = {
		nixpkgs.url = "github:NixOS/nixpkgs";
	};

	outputs =
	{ nixpkgs, self }:
	let
		pkgs = nixpkgs.legacyPackages.x86_64-linux;
	in
	{
		packages.x86_64-linux.default =
		pkgs.stdenv.mkDerivation {
			name = "notifications-server";
			src = ./.;

			buildInputs = [ pkgs.nlohmann_json ];
			nativeBuildInputs = [ pkgs.gnumake ];
		};

		nixosModules.default = import ./module.nix;
	};
}
