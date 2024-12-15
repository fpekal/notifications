{
	inputs = {
		nixpkgs.url = "github:NixOS/nixpkgs";
		server = {
			url = "path:./server";
			inputs.nixpkgs.follows = "nixpkgs";
		};

		client = {
			url = "path:./client";
			inputs.nixpkgs.follows = "nixpkgs";
		};
	};

	outputs =
	{ nixpkgs, server, client, self }:
	{
		packages.x86_64-linux = {
			notifications-server = server.packages.x86_64-linux.default;
			notifications-client = client.packages.x86_64-linux.default;
		};

		overlays.default = final: prev:
		{
			notifications-server = self.packages.x86_64-linux.notifications-server;
			notifications-client = self.packages.x86_64-linux.notifications-client;
		};
	};
}
