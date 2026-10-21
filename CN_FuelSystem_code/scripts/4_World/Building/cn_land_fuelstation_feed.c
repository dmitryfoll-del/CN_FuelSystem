modded class Land_FuelStation_Feed
{
	protected int cn_CurrentFuel = -1;

	void Land_FuelStation_Feed()
	{
		RegisterNetSyncVariableInt("cn_CurrentFuel", 0, 5000);
	}

	void cn_SetServerFuel(int cn_amount)
	{
		if (GetGame().IsServer())
		{
			cn_CurrentFuel = cn_amount;
			SetFuelAmount(cn_CurrentFuel);
			SetSynchDirty();
		}
	}

	int cn_GetServerFuel()
	{
		return cn_CurrentFuel;
	}

	override void DepleteFuel(float amount)
	{
		if (GetGame().IsServer())
		{
			float cn_litres_consumed = amount / 1000.0;

			// Вычитаем литры строго в оперативной памяти
			cn_CurrentFuel -= Math.Round(cn_litres_consumed);
			if (cn_CurrentFuel < 0) cn_CurrentFuel = 0;
			
			SetFuelAmount(cn_CurrentFuel);
			SetSynchDirty();
		}
	}

	override int GetLiquidSourceType()
	{
		if (cn_CurrentFuel <= 0)
			return LIQUID_NONE;
			
		return super.GetLiquidSourceType();
	}
}
