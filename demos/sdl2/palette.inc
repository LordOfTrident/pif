#define BLACK 1
#define WHITE 255

static uint8_t paletteData[] = {
	0x00, 0x00, 0x00, 0x04, 0x04,
	0x04, 0x0F, 0x0F, 0x0F, 0x13, 0x13, 0x13, 0x1B, 0x1B, 0x1B,
	0x1C, 0x1C, 0x1C, 0x21, 0x21, 0x21, 0x27, 0x27, 0x27, 0x2D,
	0x2D, 0x2D, 0x33, 0x33, 0x33, 0x39, 0x39, 0x39, 0x3F, 0x3F,
	0x3F, 0x45, 0x45, 0x45, 0x4B, 0x4B, 0x4B, 0x51, 0x51, 0x51,
	0x56, 0x56, 0x56, 0x5C, 0x5C, 0x5C, 0x62, 0x62, 0x62, 0x68,
	0x68, 0x68, 0x70, 0x70, 0x70, 0x79, 0x79, 0x79, 0x82, 0x82,
	0x82, 0x8B, 0x8B, 0x8B, 0x93, 0x93, 0x93, 0x9D, 0x9D, 0x9D,
	0xA6, 0xA6, 0xA6, 0xB0, 0xB0, 0xB0, 0xB9, 0xB9, 0xB9, 0xC2,
	0xC2, 0xC2, 0xCB, 0xCB, 0xCB, 0xD4, 0xD4, 0xD4, 0xDD, 0xDD,
	0xDD, 0xE6, 0xE6, 0xE6, 0x1D, 0x20, 0x1D, 0x26, 0x28, 0x25,
	0x32, 0x32, 0x32, 0x3B, 0x3C, 0x3B, 0x45, 0x48, 0x44, 0x4E,
	0x50, 0x4D, 0x58, 0x5D, 0x56, 0x61, 0x64, 0x5F, 0x6D, 0x70,
	0x68, 0x74, 0x7B, 0x70, 0x7D, 0x83, 0x79, 0x86, 0x8D, 0x82,
	0x90, 0x97, 0x8B, 0x99, 0xA1, 0x94, 0xA3, 0xAB, 0x9D, 0xAC,
	0xB5, 0xA6, 0xB5, 0xBD, 0xB0, 0xBD, 0xC4, 0xB9, 0x16, 0x1D,
	0x16, 0x1B, 0x24, 0x1B, 0x1F, 0x2B, 0x1F, 0x23, 0x33, 0x23,
	0x2B, 0x37, 0x2B, 0x2F, 0x3F, 0x2F, 0x33, 0x47, 0x33, 0x3B,
	0x4B, 0x37, 0x3F, 0x53, 0x3B, 0x43, 0x5B, 0x43, 0x4B, 0x5F,
	0x47, 0x4F, 0x67, 0x4B, 0x57, 0x6F, 0x4F, 0x5B, 0x73, 0x53,
	0x5F, 0x7B, 0x57, 0x67, 0x83, 0x5F, 0x14, 0x10, 0x24, 0x1E,
	0x1A, 0x2E, 0x28, 0x24, 0x39, 0x32, 0x2E, 0x43, 0x3B, 0x39,
	0x4E, 0x45, 0x43, 0x58, 0x4F, 0x4D, 0x63, 0x59, 0x57, 0x6D,
	0x63, 0x61, 0x78, 0x6D, 0x6B, 0x82, 0x76, 0x76, 0x8D, 0x80,
	0x80, 0x97, 0x8A, 0x8A, 0xA2, 0x94, 0x94, 0xAC, 0x3E, 0x28,
	0x0B, 0x4B, 0x32, 0x10, 0x54, 0x3B, 0x17, 0x5F, 0x43, 0x1E,
	0x67, 0x4B, 0x26, 0x6E, 0x53, 0x2F, 0x7B, 0x5F, 0x37, 0x89,
	0x6B, 0x3E, 0x96, 0x76, 0x4B, 0xA3, 0x81, 0x54, 0xAB, 0x89,
	0x5C, 0xB4, 0x92, 0x65, 0xBC, 0x9A, 0x6D, 0xC4, 0xA2, 0x75,
	0xCC, 0xAA, 0x7D, 0xD0, 0xB0, 0x85, 0x1B, 0x0F, 0x08, 0x26,
	0x14, 0x0B, 0x31, 0x1B, 0x0E, 0x3D, 0x1F, 0x0E, 0x41, 0x23,
	0x12, 0x4A, 0x25, 0x13, 0x53, 0x2B, 0x13, 0x57, 0x2F, 0x17,
	0x5F, 0x33, 0x1B, 0x67, 0x3B, 0x1F, 0x73, 0x43, 0x23, 0x7B,
	0x4B, 0x27, 0x83, 0x53, 0x2F, 0x8F, 0x5B, 0x33, 0x97, 0x63,
	0x3B, 0xA0, 0x6C, 0x40, 0xAF, 0x74, 0x4A, 0xB4, 0x7E, 0x51,
	0xC0, 0x87, 0x5B, 0xCC, 0x8F, 0x5D, 0xD5, 0x97, 0x67, 0xD8,
	0x9F, 0x73, 0xDC, 0xA7, 0x7E, 0xDF, 0xAF, 0x8A, 0xE3, 0xB7,
	0x95, 0x25, 0x14, 0x04, 0x2F, 0x18, 0x04, 0x39, 0x1C, 0x06,
	0x44, 0x21, 0x04, 0x4C, 0x24, 0x03, 0x54, 0x28, 0x00, 0x61,
	0x2F, 0x02, 0x72, 0x36, 0x00, 0x7D, 0x3F, 0x06, 0x8D, 0x4B,
	0x09, 0x9B, 0x53, 0x11, 0xA2, 0x5F, 0x15, 0xA9, 0x67, 0x1A,
	0xB4, 0x71, 0x20, 0xBC, 0x7C, 0x14, 0xCC, 0x88, 0x18, 0xDC,
	0x94, 0x1C, 0xEC, 0xA0, 0x17, 0xF4, 0xAC, 0x2F, 0xFC, 0xBB,
	0x39, 0xFC, 0xC2, 0x46, 0xFB, 0xC9, 0x53, 0xFB, 0xD0, 0x61,
	0xFB, 0xDD, 0x7B, 0x02, 0x04, 0x29, 0x02, 0x05, 0x31, 0x06,
	0x08, 0x39, 0x02, 0x05, 0x41, 0x02, 0x05, 0x4F, 0x00, 0x04,
	0x58, 0x00, 0x04, 0x60, 0x00, 0x04, 0x68, 0x04, 0x06, 0x79,
	0x02, 0x05, 0x89, 0x14, 0x17, 0x98, 0x26, 0x29, 0xA7, 0x38,
	0x3B, 0xB5, 0x4A, 0x4D, 0xC4, 0x5B, 0x5E, 0xD3, 0x6D, 0x70,
	0xE2, 0x7F, 0x82, 0xF0, 0x91, 0x94, 0xFF, 0x1F, 0x04, 0x04,
	0x27, 0x00, 0x00, 0x2F, 0x00, 0x00, 0x37, 0x00, 0x00, 0x43,
	0x00, 0x00, 0x4F, 0x00, 0x00, 0x5B, 0x00, 0x00, 0x67, 0x00,
	0x00, 0x73, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x8B, 0x00, 0x00,
	0x9B, 0x00, 0x00, 0xA7, 0x00, 0x00, 0xB9, 0x00, 0x00, 0xCA,
	0x00, 0x00, 0xDC, 0x00, 0x00, 0xED, 0x00, 0x00, 0xFF, 0x00,
	0x00, 0xFF, 0x2E, 0x2E, 0xFF, 0x5B, 0x5B, 0xFF, 0x89, 0x89,
	0xFF, 0xAB, 0xAB, 0x14, 0x10, 0x04, 0x0D, 0x18, 0x09, 0x11,
	0x21, 0x0C, 0x15, 0x29, 0x0E, 0x18, 0x32, 0x11, 0x1C, 0x39,
	0x14, 0x20, 0x41, 0x18, 0x23, 0x49, 0x1C, 0x27, 0x50, 0x1F,
	0x2C, 0x56, 0x25, 0x2E, 0x5F, 0x26, 0x33, 0x68, 0x2B, 0x3C,
	0x7A, 0x33, 0x44, 0x8B, 0x3A, 0x4D, 0x9D, 0x42, 0x55, 0xAE,
	0x49, 0x5E, 0xC0, 0x51, 0x9D, 0x33, 0x04, 0xAA, 0x41, 0x02,
	0xB9, 0x56, 0x04, 0xD5, 0x77, 0x06, 0xEA, 0x93, 0x05, 0xFF,
	0xB2, 0x06, 0xFF, 0xC3, 0x1A, 0xFF, 0xD8, 0x2D, 0x04, 0x85,
	0x04, 0x08, 0xAF, 0x08, 0x02, 0xD7, 0x02, 0x03, 0xEA, 0x03,
	0x2A, 0xFC, 0x2A, 0x79, 0xFF, 0x79, 0x03, 0x03, 0xB8, 0x0F,
	0x29, 0xDC, 0x1C, 0x50, 0xE2, 0x29, 0x77, 0xE9, 0x36, 0x9E,
	0xEF, 0x43, 0xC5, 0xF6, 0x50, 0xEC, 0xFC, 0xF4, 0x0E, 0x03,
	0xFF, 0x3F, 0x00, 0xFF, 0x5F, 0x00, 0xFF, 0x7F, 0x00, 0xFF,
	0x9F, 0x00, 0xFF, 0xC3, 0x1A, 0xFF, 0xDF, 0x00, 0x2B, 0x0D,
	0x40, 0x3D, 0x0E, 0x59, 0x5A, 0x0F, 0x7A, 0x78, 0x10, 0x9C,
	0x95, 0x10, 0xBD, 0xB2, 0x11, 0xDE, 0xC5, 0x4A, 0xE8, 0xD7,
	0x81, 0xF3, 0xEA, 0xA9, 0xFD, 0x3D, 0x10, 0x10, 0x5A, 0x24,
	0x21, 0x76, 0x38, 0x31, 0x93, 0x4D, 0x42, 0xB0, 0x61, 0x53,
	0xCC, 0x75, 0x63, 0x47, 0x35, 0x02, 0x51, 0x3F, 0x06, 0x60,
	0x48, 0x00, 0x6C, 0x50, 0x00, 0x78, 0x58, 0x00, 0x80, 0x60,
	0x00, 0x95, 0x70, 0x01, 0xB5, 0x88, 0x03, 0xD4, 0xA0, 0x04,
	0xFF, 0xFF, 0xFF,
};
