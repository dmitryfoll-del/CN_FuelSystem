modded class PlayerBase
{
	override void SetActions(string profile)
	{
		super.SetActions(profile);
		AddAction(cn_ActionCheckFuelStation);
	}
}
