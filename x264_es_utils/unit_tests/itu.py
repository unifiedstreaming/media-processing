#!/usr/bin/env python3

################################################################################
# from Rec. ITU-R BT.601-7
################################################################################

BT601_Kr = 0.299
BT601_Kg = 0.587
BT601_Kb = 0.114 # 1 - Kr - Kg

BT601_CR = [
    [BT601_Kr, BT601_Kg, BT601_Kb],
    [-1/2 * (BT601_Kr / (1 - BT601_Kb)), -1/2 * (BT601_Kg / (1 - BT601_Kb)), 1/2],
    [1/2, -1/2 * (BT601_Kg / (1 - BT601_Kr)), -1/2 * (BT601_Kb / (1 - BT601_Kr))]
]

print("Raw coefficients:")
print(BT601_CR[0])
print(BT601_CR[1])
print(BT601_CR[2])
print()

BT601_QL_y_8 = 235 - 16 # 219 quantization levels
BT601_QL_c_8 = 240 - 16 # 224 quantization levels

BT601_C8 = [
    [BT601_CR[0][0] * BT601_QL_y_8, BT601_CR[0][1] * BT601_QL_y_8, BT601_CR[0][2] * BT601_QL_y_8],
    [BT601_CR[1][0] * BT601_QL_c_8, BT601_CR[1][1] * BT601_QL_c_8, BT601_CR[1][2] * BT601_QL_c_8],
    [BT601_CR[2][0] * BT601_QL_c_8, BT601_CR[2][1] * BT601_QL_c_8, BT601_CR[2][2] * BT601_QL_c_8]
]

print("8-bit coefficients:")
print(BT601_C8[0])
print(BT601_C8[1])
print(BT601_C8[2])
print()

BT601_full_8 = 255 # full 8-bit quantization levels

BT601_C8N = [
    [BT601_C8[0][0] / BT601_full_8, BT601_C8[0][1] / BT601_full_8, BT601_C8[0][2] / BT601_full_8],
    [BT601_C8[1][0] / BT601_full_8, BT601_C8[1][1] / BT601_full_8, BT601_C8[1][2] / BT601_full_8],
    [BT601_C8[2][0] / BT601_full_8, BT601_C8[2][1] / BT601_full_8, BT601_C8[2][2] / BT601_full_8]
]

print("8-bit coefficients normalized:")
print(BT601_C8N[0])
print(BT601_C8N[1])
print(BT601_C8N[2])
print()

def rgb_to_yuv_8(r_8, g_8, b_8):
    y_8 =  16 + BT601_C8N[0][0] * r_8 + BT601_C8N[0][1] * g_8 + BT601_C8N[0][2] * b_8
    u_8 = 128 + BT601_C8N[1][0] * r_8 + BT601_C8N[1][1] * g_8 + BT601_C8N[1][2] * b_8
    v_8 = 128 + BT601_C8N[2][0] * r_8 + BT601_C8N[2][1] * g_8 + BT601_C8N[2][2] * b_8
    return y_8, u_8, v_8

print(f"8-bit black {rgb_to_yuv_8(0x00, 0x00, 0x00)}")
print(f"8-bit gray  {rgb_to_yuv_8(0x80, 0x80, 0x80)}")
print(f"8-bit white {rgb_to_yuv_8(0xff, 0xff, 0xff)}")
print()
print(f"full red    {rgb_to_yuv_8(0xff, 0x00, 0x00)}")
print(f"full green  {rgb_to_yuv_8(0x00, 0xff, 0x00)}")
print(f"full blue   {rgb_to_yuv_8(0x00, 0x00, 0xff)}")
print()


################################################################################
# from Rec. ITU-R BT.709-6
################################################################################

BT709_Kr = 0.2126
BT709_Kg = 0.7152
BT709_Kb = 0.0722 # 1 - Kr - Kg

BT709_CR = [
    [BT709_Kr, BT709_Kg, BT709_Kb],
    [-1/2 * (BT709_Kr / (1 - BT709_Kb)), -1/2 * (BT709_Kg / (1 - BT709_Kb)), 1/2],
    [1/2, -1/2 * (BT709_Kg / (1 - BT709_Kr)), -1/2 * (BT709_Kb / (1 - BT709_Kr))]
]

print("Raw coefficients:")
print(BT709_CR[0])
print(BT709_CR[1])
print(BT709_CR[2])
print()

BT709_QL_y_10 = 940 - 64 # 876 quantization levels
BT709_QL_c_10 = 960 - 64 # 896 quantization levels

BT709_C10 = [
    [BT709_CR[0][0] * BT709_QL_y_10, BT709_CR[0][1] * BT709_QL_y_10, BT709_CR[0][2] * BT709_QL_y_10],
    [BT709_CR[1][0] * BT709_QL_c_10, BT709_CR[1][1] * BT709_QL_c_10, BT709_CR[1][2] * BT709_QL_c_10],
    [BT709_CR[2][0] * BT709_QL_c_10, BT709_CR[2][1] * BT709_QL_c_10, BT709_CR[2][2] * BT709_QL_c_10]
]

print("10-bit coefficients:")
print(BT709_C10[0])
print(BT709_C10[1])
print(BT709_C10[2])
print()

BT709_full_10 = 1023 # full 10-bit quantization levels

BT709_C10N = [
    [BT709_C10[0][0] / BT709_full_10, BT709_C10[0][1] / BT709_full_10, BT709_C10[0][2] / BT709_full_10],
    [BT709_C10[1][0] / BT709_full_10, BT709_C10[1][1] / BT709_full_10, BT709_C10[1][2] / BT709_full_10],
    [BT709_C10[2][0] / BT709_full_10, BT709_C10[2][1] / BT709_full_10, BT709_C10[2][2] / BT709_full_10]
]

print("10-bit coefficients normalized:")
print(BT709_C10N[0])
print(BT709_C10N[1])
print(BT709_C10N[2])
print()

def rgb_to_yuv_10(r_10, g_10, b_10):
    y_10 =  64 + BT709_C10N[0][0] * r_10 + BT709_C10N[0][1] * g_10 + BT709_C10N[0][2] * b_10
    u_10 = 512 + BT709_C10N[1][0] * r_10 + BT709_C10N[1][1] * g_10 + BT709_C10N[1][2] * b_10
    v_10 = 512 + BT709_C10N[2][0] * r_10 + BT709_C10N[2][1] * g_10 + BT709_C10N[2][2] * b_10
    return y_10, u_10, v_10

print(f"10-bit black {rgb_to_yuv_10(0x000, 0x000, 0x000)}")
print(f"10-bit gray  {rgb_to_yuv_10(0x200, 0x200, 0x200)}")
print(f"10-bit white {rgb_to_yuv_10(0x3ff, 0x3ff, 0x3ff)}")
print()
print(f"full red    {rgb_to_yuv_10(0x3ff, 0x00, 0x00)}")
print(f"full green  {rgb_to_yuv_10(0x00, 0x3ff, 0x00)}")
print(f"full blue   {rgb_to_yuv_10(0x00, 0x00, 0x3ff)}")
print()
