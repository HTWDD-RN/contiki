#ifndef __TSL2561_PACKAGE_T_H__
#define __TSL2561_PACKAGE_T_H__

//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// T Package coefficients
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// For K_{i-1} to K_i:
// 		Lux/Ch0 = B_i - M_i * (Ch1/Ch0)
//
// For Ch1/Ch0=0.00 to 0.50
// 		Lux/Ch0=0.0304−0.062*((Ch1/Ch0)^1.4)
// 		piecewise approximation:
// 			For Ch1/Ch0=0.00 to 0.125 (K1):
// 				Lux/Ch0=0.0304−0.0272*(Ch1/Ch0)
// 			For Ch1/Ch0=0.125 to 0.250 (K2):
// 				Lux/Ch0=0.0325−0.0440*(Ch1/Ch0)
// 			For Ch1/Ch0=0.250 to 0.375 (K3):
// 				Lux/Ch0=0.0351−0.0544*(Ch1/Ch0)
// 			For Ch1/Ch0=0.375 to 0.50 (K4):
// 				Lux/Ch0=0.0381−0.0624*(Ch1/Ch0)
//
// For Ch1/Ch0=0.50 to 0.61 (K5):
// 		Lux/Ch0=0.0224−0.031*(Ch1/Ch0)
//
// For Ch1/Ch0=0.61 to 0.80 (K6):
// 		Lux/Ch0=0.0128−0.0153*(Ch1/Ch0)
//
// For Ch1/Ch0=0.80 to 1.30 (K7):
// 		Lux/Ch0=0.00146−0.00112*(Ch1/Ch0)
//
// For Ch1/Ch0>1.3 (K8):
// 		Lux/Ch0=0
//
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−

// 0.125 * 2^RATIO_SCALE
#define K1 0x0040
// 0.0304 * 2^LUX_SCALE
#define B1 0x01f2
// 0.0272 * 2^LUX_SCALE
#define M1 0x01be

// 0.250 * 2^RATIO_SCALE
#define K2 0x0080
// 0.0325 * 2^LUX_SCALE
#define B2 0x0214
// 0.0440 * 2^LUX_SCALE
#define M2 0x02d1

// 0.375 * 2^RATIO_SCALE
#define K3 0x00c0
// 0.0351 * 2^LUX_SCALE
#define B3 0x023f
// 0.0544 * 2^LUX_SCALE
#define M3 0x037b

// 0.50 * 2^RATIO_SCALE
#define K4 0x0100
// 0.0381 * 2^LUX_SCALE
#define B4 0x0270
// 0.0624 * 2^LUX_SCALE
#define M4 0x03fe

// 0.61 * 2^RATIO_SCALE
#define K5 0x0138
// 0.0224 * 2^LUX_SCALE
#define B5 0x016f
// 0.0310 * 2^LUX_SCALE
#define M5 0x01fc

// 0.80 * 2^RATIO_SCALE
#define K6 0x019a
// 0.0128 * 2^LUX_SCALE
#define B6 0x00d2
// 0.0153 * 2^LUX_SCALE
#define M6 0x00fb

// 1.3 * 2^RATIO_SCALE
#define K7 0x029a
// 0.00146 * 2^LUX_SCALE
#define B7 0x0018
// 0.00112 * 2^LUX_SCALE
#define M7 0x0012

// 1.3 * 2^RATIO_SCALE
#define K8 0x029a
// 0.000 * 2^LUX_SCALE
#define B8 0x0000
// 0.000 * 2^LUX_SCALE
#define M8 0x0000



#endif /* __TSL2561_PACKAGE_T_H__ */
