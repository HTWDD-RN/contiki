#ifndef __TSL2561_PACKAGE_CS_H__
#define __TSL2561_PACKAGE_CS_H__

//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// CS Package coefficients
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−
// For K_{i-1} to K_i:
// 		Lux/Ch0 = B_i - M_i * (Ch1/Ch0)
//
// For Ch1/Ch0=0.00 to 0.52
// 		Lux/Ch0=0.0315−0.0593*((Ch1/Ch0)^1.4)
// 		piecewise approximation:
// 			For Ch1/Ch0=0.00 to 0.13 (K1):
// 				Lux/Ch0=0.0315−0.0262*(Ch1/Ch0)
// 			For Ch1/Ch0=0.13 to 0.26 (K2):
// 				Lux/Ch0=0.0337−0.0430*(Ch1/Ch0)
// 			For Ch1/Ch0=0.26 to 0.39 (K3):
// 				Lux/Ch0=0.0363−0.0529*(Ch1/Ch0)
// 			For Ch1/Ch0=0.39 to 0.52 (K4):
// 				Lux/Ch0=0.0392−0.0605*(Ch1/Ch0)
//
// For Ch1/Ch0=0.52 to 0.65 (K5):
// 		Lux/Ch0=0.0229−0.0291*(Ch1/Ch0)
//
// For Ch1/Ch0=0.65 to 0.80 (K6):
// 		Lux/Ch0=0.00157−0.00180*(Ch1/Ch0)
//
// For Ch1/Ch0=0.80 to 1.30 (K7):
// 		Lux/Ch0=0.00338−0.00260*(Ch1/Ch0)
//
// For Ch1/Ch0>1.3 (K8):
// 		Lux/Ch0=0
//
//−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−−

// 0.130 * 2^RATIO_SCALE
#define K1 0x0043
// 0.0315 * 2^LUX_SCALE
#define B1 0x0204
// 0.0262 * 2^LUX_SCALE
#define M1 0x01ad

// 0.260 * 2^RATIO_SCALE
#define K2 0x0085
// 0.0337 * 2^LUX_SCALE
#define B2 0x0228
// 0.0430 * 2^LUX_SCALE
#define M2 0x02c1

// 0.390 * 2^RATIO_SCALE
#define K3 0x00c8
// 0.0363 * 2^LUX_SCALE
#define B3 0x0253
// 0.0529 * 2^LUX_SCALE
#define M3 0x0363

// 0.520 * 2^RATIO_SCALE
#define K4 0x010a
// 0.0392 * 2^LUX_SCALE
#define B4 0x0282
// 0.0605 * 2^LUX_SCALE
#define M4 0x03df

// 0.65 * 2^RATIO_SCALE
#define K5 0x014d
// 0.0229 * 2^LUX_SCALE
#define B5 0x0177
// 0.0291 * 2^LUX_SCALE
#define M5 0x01dd

// 0.80 * 2^RATIO_SCALE
#define K6 0x019a
// 0.0157 * 2^LUX_SCALE
#define B6 0x0101
// 0.0180 * 2^LUX_SCALE
#define M6 0x0127

// 1.3 * 2^RATIO_SCALE
#define K7 0x029a
// 0.00338 * 2^LUX_SCALE
#define B7 0x0037
// 0.00260 * 2^LUX_SCALE
#define M7 0x002b

// 1.3 * 2^RATIO_SCALE
#define K8 0x029a
// 0.000 * 2^LUX_SCALE
#define B8 0x0000
// 0.000 * 2^LUX_SCALE
#define M8 0x0000




#endif /* __TSL2561_PACKAGE_CS_H__ */
