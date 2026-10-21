class CfgPatches
{
	class cn_fuelsystem_server
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]={"cn_fuelsystem_code"};
	};
};

class CfgMods
{
	class cn_fuelsystem_server
	{
		type = "mod";
		dependencies[] = {"World"};
		class World
		{
			value = "";
			files[] = {"cn_fuelsystem_server/4_World"};
		};
	};
};
