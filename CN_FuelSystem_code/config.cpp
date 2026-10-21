class CfgPatches
{
	class cn_fuelsystem_code
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]={"DZ_Data","DZ_Scripts"};
	};
};

class CfgMods
{
	class cn_fuelsystem_code
	{
		type = "mod";
		dependencies[] = {"World"};
		class World
		{
			value = "";
			files[] = {"cn_fuelsystem_code/4_World"};
		};
	};
};
