// =============================================
// BuildDispatchArgs.CS  (Phase 3: ping-pong + DispatchIndirect)
//
//   入力側 aliveCounter(A) を読み取り、UpdateParticleIndirect.CS を
//   「生存数ぶんだけ」起動するための間接ディスパッチ引数
//   (ThreadGroupCountX, Y, Z) を生成する 1スレッドパス。
//
//   実行後、gDispatchArgs を UAV -> INDIRECT_ARGUMENT に遷移してから
//   ExecuteIndirect で使用すること。
//   count==0 のときは groupX=0 となり、DispatchIndirect は no-op になる。
// =============================================
RWStructuredBuffer<uint> gAliveCounterIn : register(u0); // 入力: 生存数 A
RWStructuredBuffer<uint> gDispatchArgs : register(u1);   // 出力: [0]=groupX [1]=groupY [2]=groupZ

static const uint kThreadsPerGroup = 1024u;

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint count = gAliveCounterIn[0];
    uint groupX = (count + kThreadsPerGroup - 1u) / kThreadsPerGroup;
    gDispatchArgs[0] = groupX;
    gDispatchArgs[1] = 1u;
    gDispatchArgs[2] = 1u;
}
