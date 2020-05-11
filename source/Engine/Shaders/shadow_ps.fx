float4 PSMain(float4 position : SV_POSITION) : SV_TARGET
{
    return float4(position.zzz, 1);
}