class CfgPatches
{
	class CN_FuelSystem_code
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]={"DZ_Data","DZ_Scripts"};
	};
};

class CfgMods
{
	class CN_FuelSystem_code
	{
		type = "mod";
		dependencies[] = {"World"};
		class World
		{
			value = "";
			files[] = {"CN_FuelSystem_code/scripts/4_World"};
		};
	};
};
