struct BufType {
	int i;
	float f;
};

StructuredBuffer<BufType> Buffer0 : register(t0);
RWStructuredBuffer<BufType> BufferOut : register(u0);

[numthreads(1, 1, 1)]
void mainCs(uint3 DTid : SV_DispatchThreadID) {

}