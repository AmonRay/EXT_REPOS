// EXP ES 1.09 Custom Shader

//=================================================================================================
//LMT - Contrast Reduction
//Match STALKER's original tonemapping by reducing the contrast
//=================================================================================================

void Contrast_Reduction( inout float3 aces)
{
	float Contrast_Amount = 1.0;
	const float mid = 0.18;
	aces = pow(aces, Contrast_Amount) * mid/pow(mid,Contrast_Amount);
	
}
