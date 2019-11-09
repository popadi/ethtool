#ifndef QSFP_DD_H__
#define QSFP_DD_H__

#define QSFP_DD_PAG_SIZE			0x80
#define QSFP_DD_EEPROM_5PAG			(0x80 * 6)
#define QSFP_DD_MAX_CHANNELS			0x08
#define QSFP_DD_MAX_DESC_SIZE			0x2A
#define QSFP_DD_READ_TX				0x00
#define QSFP_DD_READ_RX				0x01

/* Struct for the current/power of a channel */
struct qsfp_dd_channel_diags {
	__u16 bias_cur;
	__u16 rx_power;
	__u16 tx_power;
};

struct qsfp_dd_diags {
	/* Voltage in 0.1mV units; the first 4 elements represent
	 * the high/low alarm, high/low warning and the last one
	 * represent the current voltage of the module.
	 */
	__u16 sfp_voltage[4];

	/**
	 * Temperature in 16-bit signed 1/256 Celsius; the first 4
	 * elements represent the high/low alarm, high/low warning
	 * and the last one represent the current temp of the module.
	 */
	__s16 sfp_temp[4];

	/* Tx bias current in 2uA units */
	__u16 bias_cur[4];

	/* Measured TX Power */
	__u16 tx_power[4];

	/* Measured RX Power */
	__u16 rx_power[4];

	/* Rx alarms and warnings */
	bool rxaw[QSFP_DD_MAX_CHANNELS][4];

	/* Tx alarms and warnings */
	bool txaw[QSFP_DD_MAX_CHANNELS][4];

	struct qsfp_dd_channel_diags scd[QSFP_DD_MAX_CHANNELS];
};

#define HA					0
#define LA					1
#define HW					2
#define LW					3

/* Identifier and revision compliance (Page 0) */
#define	QSFP_DD_ID_OFFSET			0x00
#define QSFP_DD_REV_COMPLIANCE_OFFSET		0x01

#define QSFP_DD_MODULE_TYPE_OFFSET		0x55
#define QSFP_DD_MT_MMF				0x01
#define QSFP_DD_MT_SMF				0x02

/* Module-Level Monitors (Page 0) */
#define QSFP_DD_CURR_TEMP_OFFSET		0x0E
#define QSFP_DD_CURR_CURR_OFFSET		0x10

#define QSFP_DD_CTOR_OFFSET			0xCB

/* Vendor related information (Page 0) */
#define QSFP_DD_VENDOR_NAME_START_OFFSET	0x81
#define QSFP_DD_VENDOR_NAME_END_OFFSET		0x90

#define QSFP_DD_VENDOR_OUI_OFFSET		0x91

#define QSFP_DD_VENDOR_PN_START_OFFSET		0x94
#define QSFP_DD_VENDOR_PN_END_OFFSET		0xA3

#define QSFP_DD_VENDOR_REV_START_OFFSET		0xA4
#define QSFP_DD_VENDOR_REV_END_OFFSET		0xA5

#define QSFP_DD_VENDOR_SN_START_OFFSET		0xA6
#define QSFP_DD_VENDOR_SN_END_OFFSET		0xB5

#define QSFP_DD_DATE_YEAR_OFFSET		0xB6
#define QSFP_DD_DATE_VENDOR_LOT_OFFSET		0xBD

/* CLEI Code (Page 0) */
#define QSFP_DD_CLEI_PRESENT_BYTE		0x02
#define QSFP_DD_CLEI_PRESENT_MASK		0x20
#define QSFP_DD_CLEI_START_OFFSET		0xBE
#define QSFP_DD_CLEI_END_OFFSET			0xC7

/* Cable assembly length */
#define QSFP_DD_CBL_ASM_LEN_OFFSET		0xCA
#define QSFP_DD_6300M_MAX_LEN			0xFF

/* Cable length with multiplier */
#define QSFP_DD_MULTIPLIER_00			0x00
#define QSFP_DD_MULTIPLIER_01			0x40
#define QSFP_DD_MULTIPLIER_10			0x80
#define QSFP_DD_MULTIPLIER_11			0xC0
#define QSFP_DD_LEN_MUL_MASK			0xC0
#define QSFP_DD_LEN_VAL_MASK			0x3F

/* Module power characteristics */
#define QSFP_DD_PWR_CLASS_OFFSET		0xC8
#define QSFP_DD_PWR_MAX_POWER_OFFSET		0xC9
#define QSFP_DD_PWR_CLASS_MASK			0xE0
#define QSFP_DD_PWR_CLASS_1			0x00
#define QSFP_DD_PWR_CLASS_2			0x01
#define QSFP_DD_PWR_CLASS_3			0x02
#define QSFP_DD_PWR_CLASS_4			0x03
#define QSFP_DD_PWR_CLASS_5			0x04
#define QSFP_DD_PWR_CLASS_6			0x05
#define QSFP_DD_PWR_CLASS_7			0x06
#define QSFP_DD_PWR_CLASS_8			0x07

/* Copper cable attenuation */
#define QSFP_DD_COPPER_ATT_5GHZ			0xCC
#define QSFP_DD_COPPER_ATT_7GHZ			0xCD
#define QSFP_DD_COPPER_ATT_12P9GHZ		0xCE
#define QSFP_DD_COPPER_ATT_25P8GHZ		0xCF

/* Cable assembly lane */
#define QSFP_DD_CABLE_ASM_NEAR_END_OFFSET	0xD2
#define QSFP_DD_CABLE_ASM_FAR_END_OFFSET	0xD3

/* Media interface technology */
#define QSFP_DD_MEDIA_INTF_TECH_OFFSET		0xD4
#define QSFP_DD_850_VCSEL			0x00
#define QSFP_DD_1310_VCSEL			0x01
#define QSFP_DD_1550_VCSEL			0x02
#define QSFP_DD_1310_FP				0x03
#define QSFP_DD_1310_DFB			0x04
#define QSFP_DD_1550_DFB			0x05
#define QSFP_DD_1310_EML			0x06
#define QSFP_DD_1550_EML			0x07
#define QSFP_DD_OTHERS				0x08
#define QSFP_DD_1490_DFB			0x09
#define QSFP_DD_COPPER_UNEQUAL			0x0A
#define QSFP_DD_COPPER_PASS_EQUAL		0x0B
#define QSFP_DD_COPPER_NF_EQUAL			0x0C
#define QSFP_DD_COPPER_F_EQUAL			0x0D
#define QSFP_DD_COPPER_N_EQUAL			0x0E
#define QSFP_DD_COPPER_LINEAR_EQUAL		0x0F

/*-----------------------------------------------------------------------
 * For optical connected cables (the eeprom length is equal to  640 bytes
 * = QSFP_DD_EEPROM_WITH_OPTICAL), the memory has the following format:
 * Bytes   0-127: page  0 (lower)
 * Bytes 128-255: page  0 (higher)
 * Bytes 256-383: page  1 (higher)
 * Bytes 384-511: page  2 (higher)
 * Bytes 512-639: page 16 (higher)
 * Bytes 640-768: page 17 (higher)
 *
 * Since for pages with an index > 0 the lower part is missing from the memory,
 * but the offset values are still in the [128, 255) range, the real offset in
 * the eeprom memory must be calculated as following:
 * RealOffset = PageIndex * 0x80 + LocalOffset

 * The page index is the index of the page, starting from 0: page 0 has index
 * 1, page 1 has index 1, page 2 has index 2, page 16 has index 3 and page 17
 * has index 4.
 */

/*-----------------------------------------------------------------------
 * Upper Memory Page 0x01: contains advertising fields that define properties
 * that are unique to active modules and cable assemblies.
 * RealOffset = 1 * 0x80 + LocalOffset
 */
#define PAG01H_OFFSET				(0x01 * 0x80)

/* Supported Link Length (Page 1) */
#define QSFP_DD_SMF_LEN_OFFSET			(PAG01H_OFFSET + 0x84)
#define QSFP_DD_OM5_LEN_OFFSET			(PAG01H_OFFSET + 0x85)
#define QSFP_DD_OM4_LEN_OFFSET			(PAG01H_OFFSET + 0x86)
#define QSFP_DD_OM3_LEN_OFFSET			(PAG01H_OFFSET + 0x87)
#define QSFP_DD_OM2_LEN_OFFSET			(PAG01H_OFFSET + 0x88)

/* Wavelength (Page 1) */
#define QSFP_DD_NOM_WAVELENGTH_MSB		(PAG01H_OFFSET + 0x8A)
#define QSFP_DD_NOM_WAVELENGTH_LSB		(PAG01H_OFFSET + 0x8B)
#define QSFP_DD_WAVELENGTH_TOL_MSB		(PAG01H_OFFSET + 0x8C)
#define QSFP_DD_WAVELENGTH_TOL_LSB		(PAG01H_OFFSET + 0x8D)

/* Signal integrity controls */
#define QSFP_DD_SIG_INTEG_TX_OFFSET		(PAG01H_OFFSET + 0xA1)
#define QSFP_DD_SIG_INTEG_RX_OFFSET		(PAG01H_OFFSET + 0xA2)

/*-----------------------------------------------------------------------
 * Upper Memory Page 0x02: contains module defined threshdolds and lane-
 * specific monitors.
 * RealOffset = 2 * 0x80 + LocalOffset
 */
#define PAG02H_OFFSET				(0x02 * 0x80)
#define QSFP_DD_TEMP_THRS_START_OFFSET		(PAG02H_OFFSET + 0x80)
#define QSFP_DD_VOLT_THRS_START_OFFSET		(PAG02H_OFFSET + 0x88)
#define QSFP_DD_TXPW_THRS_START_OFFSET		(PAG02H_OFFSET + 0xB0)
#define QSFP_DD_TXBI_THRS_START_OFFSET		(PAG02H_OFFSET + 0xB8)
#define QSFP_DD_RXPW_THRS_START_OFFSET		(PAG02H_OFFSET + 0xC0)

/*-----------------------------------------------------------------------
 * Upper Memory Page 0x10: contains dynamic control bytes.
 * RealOffset = 3 * 0x80 + LocalOffset
 */
#define PAG16H_OFFSET				(0x03 * 0x80)

/*-----------------------------------------------------------------------
 * Upper Memory Page 0x11: contains lane dynamic status bytes.
 * RealOffset = 4 * 0x80 + LocalOffset
 */
#define PAG11H_OFFSET				(0x04 * 0x80)
#define QSFP_DD_TX_PWR_START_OFFSET		(PAG11H_OFFSET + 0x9A)
#define QSFP_DD_TX_BIAS_START_OFFSET		(PAG11H_OFFSET + 0xAA)
#define QSFP_DD_RX_PWR_START_OFFSET		(PAG11H_OFFSET + 0xBA)

/* HA = High Alarm; LA = Low Alarm
 * HW = High Warning; LW = Low Warning
 */
#define QSFP_DD_TX_HA_OFFSET			(PAG11H_OFFSET + 0x8B)
#define QSFP_DD_TX_LA_OFFSET			(PAG11H_OFFSET + 0x8C)
#define QSFP_DD_TX_HW_OFFSET			(PAG11H_OFFSET + 0x8D)
#define QSFP_DD_TX_LW_OFFSET			(PAG11H_OFFSET + 0x8E)

#define QSFP_DD_RX_HA_OFFSET			(PAG11H_OFFSET + 0x95)
#define QSFP_DD_RX_LA_OFFSET			(PAG11H_OFFSET + 0x96)
#define QSFP_DD_RX_HW_OFFSET			(PAG11H_OFFSET + 0x97)
#define QSFP_DD_RX_LW_OFFSET			(PAG11H_OFFSET + 0x98)

#define YESNO(x) (((x) != 0) ? "Yes" : "No")
#define ONOFF(x) (((x) != 0) ? "On" : "Off")

void qsfp_dd_show_all(const __u8 *id, __u32 eeprom_len);

#endif /* QSFP_DD_H__ */
