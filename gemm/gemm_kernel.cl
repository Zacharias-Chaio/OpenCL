
__kernel void gemm_block4x4_F32(__global float4 *inputA,
                        __global float4 *inputB,
                        __global float4* output,
            uint M, uint K, uint N)
{
    int gidx = get_global_id(0);
    int gidy = get_global_id(1) << 2;

    float4 sum0 = (float4)(0);
    float4 sum1 = (float4)(0);
    float4 sum2 = (float4)(0);
    float4 sum3 = (float4)(0);

    /* Vectorization of input Matrices reduces their width by a factor of 4 */
    int4 widthA = (int4)(K, K, K, K) >> 2;
    int widthB = N >> 2;
    int4 offset = (int4)(0, 1, 2, 3);
    int4 offsetA = (offset + gidy) * widthA;
    //int4 offsetB = mad_sat(offset, widthB, gidx);
    int4 offsetC = (offset + gidy) * widthB + gidx;

    __global float4 *inputB0 = (__global float4 *)inputB + gidx;
    __global float4 *inputB1 = (__global float4 *)inputB0 + widthB;
    __global float4 *inputB2 = (__global float4 *)inputB1 + widthB;
    __global float4 *inputB3 = (__global float4 *)inputB2 + widthB;

    for(int i = 0; i < widthA.x; i++)
    {
        int4 offsetX = i + offsetA;
        int offsetY = i * N;
        float4 tempA0 = inputA[offsetX.x];
        float4 tempA1 = inputA[offsetX.y];
        float4 tempA2 = inputA[offsetX.z];
        float4 tempA3 = inputA[offsetX.w];

#if 0
        int4 offsetY = mad_sat(i, N, offsetB);
        float4 tempB0 = inputB[offsetY.x];
        float4 tempB1 = inputB[offsetY.y];
        float4 tempB2 = inputB[offsetY.z];
        float4 tempB3 = inputB[offsetY.w];
#endif
        float4 tempB0 = inputB0[offsetY];
        float4 tempB1 = inputB1[offsetY];
        float4 tempB2 = inputB2[offsetY];
        float4 tempB3 = inputB3[offsetY];

        sum0 = sum0 + tempA0.xxxx * tempB0 + tempA0.yyyy * tempB1 + tempA0.zzzz * tempB2 + tempA0.wwww * tempB3;
        sum1 = sum1 + tempA1.xxxx * tempB0 + tempA1.yyyy * tempB1 + tempA1.zzzz * tempB2 + tempA1.wwww * tempB3;
        sum2 = sum2 + tempA2.xxxx * tempB0 + tempA2.yyyy * tempB1 + tempA2.zzzz * tempB2 + tempA2.wwww * tempB3;
        sum3 = sum3 + tempA3.xxxx * tempB0 + tempA3.yyyy * tempB1 + tempA3.zzzz * tempB2 + tempA3.wwww * tempB3;
    }
    output[offsetC.x] = sum0;
    output[offsetC.y] = sum1;
    output[offsetC.z] = sum2;
    output[offsetC.w] = sum3;
}