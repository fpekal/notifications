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
			name = "notifications-client";
			src = ./.;

			nativeBuildInputs = [ pkgs.gnumake pkgs.makeWrapper ];

			patchPhase = ''
			substituteInPlace client.cpp \
				--replace-fail "/usr/bin/twmnc" "${pkgs.twmn}/bin/twmnc"
			'';

			fixupPhase = ''
			wrapProgram $out/bin/notifications_client \
				--prefix PATH : ${pkgs.twmn}/bin
			'';
		};
	};
}
