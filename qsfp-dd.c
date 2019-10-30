/**
 * Description:
 *
 * This module adds QSFP-DD support to ethtool. The changes are similar to
 * the ones already existing in qsfp.c, but customized to use the memory
 * addresses and logic as defined in the specification's document.
 *
 * Page 0x00 (lower and higher memory) are always implemented, so the ethtool
 * expects at least 256 bytes if the identifier matches the one for QSFP-DD.
 * For optical connected cables, additional pages are usually available (they
 * contain module defined thresholds or lane diagnostic information). In
 * this case, ethtool expects to receive 768 bytes in the following format:
 *
 *     +----------+----------+----------+----------+----------+----------+
 *     |   Page   |   Page   |   Page   |   Page   |   Page   |   Page   |
 *     |   0x00   |   0x00   |   0x01   |   0x02   |   0x10   |   0x11   |
 *     |  (lower) | (higher) | (higher) | (higher) | (higher) | (higher) |
 *     |   128b   |   128b   |   128b   |   128b   |   128b   |   128b   |
 *     +----------+----------+----------+----------+----------+----------+
 */

#include <stdio.h>
#include <math.h>
#include "internal.h"
#include "sff-common.h"
#include "qsfp-dd.h"

static void qsfp_dd_show_identifier(const __u8 *id)
{
	sff8024_show_identifier(id, QSFP_DD_ID_OFFSET);
}

static void qsfp_dd_show_connector(const __u8 *id)
{
	sff8024_show_connector(id, QSFP_DD_CTOR_OFFSET);
}

static void qsfp_dd_show_oui(const __u8 *id)
{
	sff8024_show_oui(id, QSFP_DD_VENDOR_OUI_OFFSET);
}

/**
 * Print the revision compliance. Relevant documents:
 * [1] CMIS Rev. 3, pag. 45, section 1.7.2.1, Table 18
 * [2] CMIS Rev. 4, pag. 81, section 8.2.1, Table 8-2
 */
static void qsfp_dd_show_rev_compliance(const __u8 *id)
{
	__u8 rev = id[QSFP_DD_REV_COMPLIANCE_OFFSET];
	int major = (rev >> 4) & 0x0F;
	int minor = rev & 0x0F;

	printf("\t%-41s : Rev. %d.%d\n", "Revision compliance", major, minor);
}

/**
 * Print information about the device's power consumption.
 * Relevant documents:
 * [1] CMIS Rev. 3, pag. 59, section 1.7.3.9, Table 30
 * [2] CMIS Rev. 4, pag. 94, section 8.3.9, Table 8-18
 * [3] QSFP-DD Hardware Rev 5.0, pag. 22, section 4.2.1
 */
static void qsfp_dd_show_power_info(const __u8 *id)
{
	float max_power = 0.0f;
	__u8 base_power = 0;
	__u8 power_class;

	/* Get the power class (first 3 most significat bytes) */
	power_class = (id[QSFP_DD_PWR_CLASS_OFFSET] >> 5) & 0x07;

	/* Get the base power in multiples of 0.25W */
	base_power = id[QSFP_DD_PWR_MAX_POWER_OFFSET];
	max_power = base_power * 0.25f;

	printf("\t%-41s : %d\n", "Power class", power_class + 1);
	printf("\t%-41s : %.02fW\n", "Max power", max_power);
}

/**
 * Print the cable assembly length, for both passive copper and active
 * optical or electrical cables. The base length (bits 5-0) must be
 * multiplied with the SMF length multiplier (bits 7-6) to obtain the
 * correct value. Relevant documents:
 * [1] CMIS Rev. 3, pag. 59, section 1.7.3.10, Table 31
 * [2] CMIS Rev. 4, pag. 94, section 8.3.10, Table 8-19
 */
static void qsfp_dd_show_cbl_asm_len(const __u8 *id)
{
	static const char *fn = "Cable assembly length";
	float mul = 1.0f;
	float val = 0.0f;

	/* Check if max length */
	if (id[QSFP_DD_CBL_ASM_LEN_OFFSET] == QSFP_DD_6300M_MAX_LEN) {
		printf("\t%-41s : > 6.3km\n", fn);
		return;
	}

	/* Get the multiplier from the first two bits */
	switch (id[QSFP_DD_CBL_ASM_LEN_OFFSET] & QSFP_DD_LEN_MUL_MASK) {
	case QSFP_DD_MULTIPLIER_00:
		mul = 0.1f;
		break;
	case QSFP_DD_MULTIPLIER_01:
		mul = 1.0f;
		break;
	case QSFP_DD_MULTIPLIER_10:
		mul = 10.0f;
		break;
	case QSFP_DD_MULTIPLIER_11:
		mul = 100.0f;
		break;
	default:
		break;
	}

	/* Get base value from first 6 bits and multiply by mul */
	val = (id[QSFP_DD_CBL_ASM_LEN_OFFSET] & QSFP_DD_LEN_VAL_MASK);
	val = (float)val * mul;
	printf("\t%-41s : %0.2fkm\n", fn, val);
}

/**
 * Print the length for SMF fiber. The base length (bits 5-0) must be
 * multiplied with the SMF length multiplier (bits 7-6) to obtain the
 * correct value. Relevant documents:
 * [1] CMIS Rev. 3, pag. 63, section 1.7.4.2, Table 39
 * [2] CMIS Rev. 4, pag. 99, section 8.4.2, Table 8-27
 */
static void qsfp_dd_print_smf_cbl_len(const __u8 *id)
{
	static const char *fn = "Length (SMF)";
	float mul = 1.0f;
	float val = 0.0f;

	/* Get the multiplier from the first two bits */
	switch (id[QSFP_DD_SMF_LEN_OFFSET] & QSFP_DD_LEN_MUL_MASK) {
	case QSFP_DD_MULTIPLIER_00:
		mul = 0.1f;
		break;
	case QSFP_DD_MULTIPLIER_01:
		mul = 1.0f;
		break;
	default:
		break;
	}

	/* Get base value from first 6 bits and multiply by mul */
	val = (id[QSFP_DD_SMF_LEN_OFFSET] & QSFP_DD_LEN_VAL_MASK);
	val = (float)val * mul;
	printf("\t%-41s : %0.2fkm\n", fn, val);
}

/**
 * Print relevant signal integrity control properties. Relevant documents:
 * [1] CMIS Rev. 3, pag. 71, section 1.7.4.10, Table 46
 * [2] CMIS Rev. 4, pag. 105, section 8.4.10, Table 8-34
 */
static void qsfp_dd_show_sig_integrity(const __u8 *id)
{
	/* CDR Bypass control: 2nd bit from each byte */
	printf("\t%-41s : ", "Tx CDR bypass control");
	printf("%s\n", YESNO(id[QSFP_DD_SIG_INTEG_TX_OFFSET] & 0x02));

	printf("\t%-41s : ", "Rx CDR bypass control");
	printf("%s\n", YESNO(id[QSFP_DD_SIG_INTEG_RX_OFFSET] & 0x02));

	/* CDR Implementation: 1st bit from each byte */
	printf("\t%-41s : ", "Tx CDR");
	printf("%s\n", YESNO(id[QSFP_DD_SIG_INTEG_TX_OFFSET] & 0x01));

	printf("\t%-41s : ", "Rx CDR");
	printf("%s\n", YESNO(id[QSFP_DD_SIG_INTEG_RX_OFFSET] & 0x01));
}

/**
 * Print relevant media interface technology info. Relevant documents:
 * [1] CMIS Rev. 3
 * --> pag. 61, section 1.7.3.14, Table 36
 * --> pag. 64, section 1.7.4.3, 1.7.4.4
 * [2] CMIS Rev. 4
 * --> pag. 97, section 8.3.14, Table 8-24
 * --> pag. 98, section 8.4, Table 8-25
 * --> page 100, section 8.4.3, 8.4.4
 */
static void qsfp_dd_show_mit_compliance(const __u8 *id)
{
	static const char *cc = " (Copper cable,";

	printf("\t%-41s : 0x%02x", "Transmitter technology",
	       id[QSFP_DD_MEDIA_INTF_TECH_OFFSET]);

	switch (id[QSFP_DD_MEDIA_INTF_TECH_OFFSET]) {
	case QSFP_DD_850_VCSEL:
		printf(" (850 nm VCSEL)\n");
		break;
	case QSFP_DD_1310_VCSEL:
		printf(" (1310 nm VCSEL)\n");
		break;
	case QSFP_DD_1550_VCSEL:
		printf(" (1550 nm VCSEL)\n");
		break;
	case QSFP_DD_1310_FP:
		printf(" (1310 nm FP)\n");
		break;
	case QSFP_DD_1310_DFB:
		printf(" (1310 nm DFB)\n");
		break;
	case QSFP_DD_1550_DFB:
		printf(" (1550 nm DFB)\n");
		break;
	case QSFP_DD_1310_EML:
		printf(" (1310 nm EML)\n");
		break;
	case QSFP_DD_1550_EML:
		printf(" (1550 nm EML)\n");
		break;
	case QSFP_DD_OTHERS:
		printf(" (Others/Undefined)\n");
		break;
	case QSFP_DD_1490_DFB:
		printf(" (1490 nm DFB)\n");
		break;
	case QSFP_DD_COPPER_UNEQUAL:
		printf("%s unequalized)\n", cc);
		break;
	case QSFP_DD_COPPER_PASS_EQUAL:
		printf("%s passive equalized)\n", cc);
		break;
	case QSFP_DD_COPPER_NF_EQUAL:
		printf("%s near and far end limiting active equalizers)\n", cc);
		break;
	case QSFP_DD_COPPER_F_EQUAL:
		printf("%s far end limiting active equalizers)\n", cc);
		break;
	case QSFP_DD_COPPER_N_EQUAL:
		printf("%s near end limiting active equalizers)\n", cc);
		break;
	case QSFP_DD_COPPER_LINEAR_EQUAL:
		printf("%s linear active equalizers)\n", cc);
		break;
	}

	if (id[QSFP_DD_MEDIA_INTF_TECH_OFFSET] >= QSFP_DD_COPPER_UNEQUAL) {
		printf("\t%-41s : %udb\n", "Attenuation at 5GHz",
		       id[QSFP_DD_COPPER_ATT_5GHZ]);
		printf("\t%-41s : %udb\n", "Attenuation at 7GHz",
		       id[QSFP_DD_COPPER_ATT_7GHZ]);
		printf("\t%-41s : %udb\n", "Attenuation at 12.9GHz",
		       id[QSFP_DD_COPPER_ATT_12P9GHZ]);
		printf("\t%-41s : %udb\n", "Attenuation at 25.8GHz",
		       id[QSFP_DD_COPPER_ATT_25P8GHZ]);
	} else {
		printf("\t%-41s : %.3lfnm\n", "Laser wavelength",
		       (((id[QSFP_DD_NOM_WAVELENGTH_MSB] << 8) |
				id[QSFP_DD_NOM_WAVELENGTH_LSB]) * 0.05));
		printf("\t%-41s : %.3lfnm\n", "Laser wavelength tolerance",
		       (((id[QSFP_DD_WAVELENGTH_TOL_MSB] << 8) |
		       id[QSFP_DD_WAVELENGTH_TOL_LSB]) * 0.005));
	}
}

/**
 * Read the high/low alarms or warnings for a specific channel. This
 * information is found in the i'th bit of each byte associated with
 * one of the aforementioned properties. A value greater than zero
 * means the alarm/warning is turned on.
 * The values are stored in the qsfp_dd_diags structure, in the rxaw
 * or txaw array (each element corresponds to an alarm/warning).
 */
static void qsfp_dd_read_aw_for_channel(const __u8 *id, int ch, int mode,
					struct qsfp_dd_diags * const sd)
{
	__u8 cmsk = (1 << ch);

	if (mode == QSFP_DD_READ_TX) {
		sd->txaw[ch][HA] = id[QSFP_DD_TX_HA_OFFSET] & cmsk;
		sd->txaw[ch][LA] = id[QSFP_DD_TX_LA_OFFSET] & cmsk;
		sd->txaw[ch][HW] = id[QSFP_DD_TX_HW_OFFSET] & cmsk;
		sd->txaw[ch][LW] = id[QSFP_DD_TX_LW_OFFSET] & cmsk;
	} else {
		sd->rxaw[ch][HA] = id[QSFP_DD_RX_HA_OFFSET] & cmsk;
		sd->rxaw[ch][LA] = id[QSFP_DD_RX_LA_OFFSET] & cmsk;
		sd->rxaw[ch][HW] = id[QSFP_DD_RX_HW_OFFSET] & cmsk;
		sd->rxaw[ch][LW] = id[QSFP_DD_RX_LW_OFFSET] & cmsk;
	}
}

/*
 * 2-byte internal temperature conversions:
 * First byte is a signed 8-bit integer, which is the temp decimal part
 * Second byte are 1/256th of degree, which are added to the dec part.
 */
#define OFFSET_TO_TEMP(offset) ((__s16)OFFSET_TO_U16(offset))

/**
 * Get and parse relevant diagnostic information for the current module.
 * These are stored, for every channel, in a qsfp_dd_diags structure.
 */
static void
qsfp_dd_parse_diagnostics(const __u8 *id, struct qsfp_dd_diags *const sd)
{
	__u16 rx_power_offset;
	__u16 tx_power_offset;
	__u16 tx_bias_offset;
	__u16 temp_offset;
	__u16 volt_offset;
	int i;

	for (i = 0; i < QSFP_DD_MAX_CHANNELS; ++i) {
		/*
		 * Add Tx/Rx output/input optical power relevant information.
		 * To access the info for the ith lane, we have to skip i * 2
		 * bytes starting from the offset of the first lane for that
		 * specific channel property.
		 */
		tx_bias_offset = QSFP_DD_TX_BIAS_START_OFFSET + (i << 1);
		rx_power_offset = QSFP_DD_RX_PWR_START_OFFSET + (i << 1);
		tx_power_offset = QSFP_DD_TX_PWR_START_OFFSET + (i << 1);

		sd->scd[i].bias_cur = OFFSET_TO_U16(tx_bias_offset);
		sd->scd[i].rx_power = OFFSET_TO_U16(rx_power_offset);
		sd->scd[i].tx_power = OFFSET_TO_U16(tx_power_offset);

		/* Add alarms/warnings related info */
		qsfp_dd_read_aw_for_channel(id, i, QSFP_DD_READ_TX, sd);
		qsfp_dd_read_aw_for_channel(id, i, QSFP_DD_READ_RX, sd);
	}

	/**
	 * Gather Module-Level Monitor Thresholds and Lane-specific Monitor
	 * Thresholds. These values are stored in two bytes (MSB, LSB) in
	 * the following order: HA, LA, HW, LW, thus we only need the start
	 * offset for each property.
	 */
	for (i = 0; i < 4; ++i) {
		tx_power_offset = QSFP_DD_TXPW_THRS_START_OFFSET + (i << 1);
		sd->tx_power[i] = OFFSET_TO_U16(tx_power_offset);

		rx_power_offset = QSFP_DD_RXPW_THRS_START_OFFSET + (i << 1);
		sd->rx_power[i] = OFFSET_TO_U16(rx_power_offset);

		tx_bias_offset = QSFP_DD_TXBI_THRS_START_OFFSET + (i << 1);
		sd->bias_cur[i] = OFFSET_TO_U16(tx_bias_offset);

		temp_offset = QSFP_DD_TEMP_THRS_START_OFFSET + (i << 1);
		sd->sfp_temp[i] = OFFSET_TO_TEMP(temp_offset);

		volt_offset = QSFP_DD_VOLT_THRS_START_OFFSET + (i << 1);
		sd->sfp_voltage[i] = OFFSET_TO_U16(volt_offset);
	}
}

/**
 * Print the Module-Level Monitor Thresholds and the Lane-specific
 * Monitor Thresholds. This is the same function as the one in the
 * sff-common.c file, but is using a struct qsfp_dd_diags as a pa-
 * rameter.
 */
static void qsfp_dd_show_thresholds(const struct qsfp_dd_diags sd)
{
	PRINT_BIAS("Laser bias current high alarm threshold",
		   sd.bias_cur[HA]);
	PRINT_BIAS("Laser bias current low alarm threshold",
		   sd.bias_cur[LA]);
	PRINT_BIAS("Laser bias current high warning threshold",
		   sd.bias_cur[HW]);
	PRINT_BIAS("Laser bias current low warning threshold",
		   sd.bias_cur[LW]);

	PRINT_xX_PWR("Laser output power high alarm threshold",
		     sd.tx_power[HA]);
	PRINT_xX_PWR("Laser output power low alarm threshold",
		     sd.tx_power[LA]);
	PRINT_xX_PWR("Laser output power high warning threshold",
		     sd.tx_power[HW]);
	PRINT_xX_PWR("Laser output power low warning threshold",
		     sd.tx_power[LW]);

	PRINT_TEMP("Module temperature high alarm threshold",
		   sd.sfp_temp[HA]);
	PRINT_TEMP("Module temperature low alarm threshold",
		   sd.sfp_temp[LA]);
	PRINT_TEMP("Module temperature high warning threshold",
		   sd.sfp_temp[HW]);
	PRINT_TEMP("Module temperature low warning threshold",
		   sd.sfp_temp[LW]);

	PRINT_VCC("Module voltage high alarm threshold",
		  sd.sfp_voltage[HA]);
	PRINT_VCC("Module voltage low alarm threshold",
		  sd.sfp_voltage[LA]);
	PRINT_VCC("Module voltage high warning threshold",
		  sd.sfp_voltage[HW]);
	PRINT_VCC("Module voltage low warning threshold",
		  sd.sfp_voltage[LW]);

	PRINT_xX_PWR("Laser rx power high alarm threshold",
		     sd.rx_power[HA]);
	PRINT_xX_PWR("Laser rx power low alarm threshold",
		     sd.rx_power[LA]);
	PRINT_xX_PWR("Laser rx power high warning threshold",
		     sd.rx_power[HW]);
	PRINT_xX_PWR("Laser rx power low warning threshold",
		     sd.rx_power[LW]);
}

/**
 * Print relevant lane specific monitor information for each of
 * the 8 available channels. Relevant documents:
 * [1] CMIS Rev. 3:
 * --> pag. 50, section 1.7.2.4, Table 22
 * --> pag. 53, section 1.7.2.7, Table 26
 * --> pag. 76, section 1.7.5.1, Table 50
 * --> pag. 78, section 1.7.5.2, Table 51
 * --> pag. 98, section 1.7.7.2, Table 67
 *
 * [2] CMIS Rev. 4:
 * --> pag. 84, section 8.2.4, Table 8-6
 * --> pag. 89, section 8.2.9, Table 8-12
 * --> pag. 112, section 8.5.1/2, Table 8-41/42
 * --> pag. 137, section 8.8.2, Table 8-60/61
 * --> pag. 140, section 8.8.3, Table 8-62
 */
static void qsfp_dd_show_sig_optical_pwr(const __u8 *id, __u32 eeprom_len)
{
	static const char * const aw_strings[] = {
		"%s power high alarm   (Channel %d)",
		"%s power low alarm    (Channel %d)",
		"%s power high warning (Channel %d)",
		"%s power low warning  (Channel %d)"
	};
	__u8 module_type = id[QSFP_DD_MODULE_TYPE_OFFSET];
	char field_desc[QSFP_DD_MAX_DESC_SIZE];
	struct qsfp_dd_diags sd = { { 0 } };
	const char *cs = "%s (Channel %d)";
	int i, j;

	/* Print current temperature & voltage */
	PRINT_TEMP("Module temperature",
		   OFFSET_TO_TEMP(QSFP_DD_CURR_TEMP_OFFSET));
	PRINT_VCC("Module voltage",
		  OFFSET_TO_U16(QSFP_DD_CURR_CURR_OFFSET));

	/**
	 * The thresholds and the high/low alarms/warnings are available
	 * only if an optical interface (MMF/SMF) is present (if this is
	 * the case, it means that 5 pages are available).
	 */
	if (module_type != QSFP_DD_MT_MMF &&
	    module_type != QSFP_DD_MT_SMF &&
	    eeprom_len != QSFP_DD_EEPROM_5PAG)
		return;

	/* Extract the diagnostic variables */
	qsfp_dd_parse_diagnostics(id, &sd);

	/* Print Tx bias current monitor values */
	for (i = 0; i < QSFP_DD_MAX_CHANNELS; ++i) {
		snprintf(field_desc, QSFP_DD_MAX_DESC_SIZE, cs,
			 "Tx bias current monitor", i + 1);
		PRINT_BIAS(field_desc, sd.scd[i].bias_cur);
	}

	/* Print Tx output optical power values */
	for (i = 0; i < QSFP_DD_MAX_CHANNELS; ++i) {
		snprintf(field_desc, QSFP_DD_MAX_DESC_SIZE, cs,
			 "Tx output optical power", i + 1);
		PRINT_xX_PWR(field_desc, sd.scd[i].tx_power);
	}

	/* Print Rx output optical power values */
	for (i = 0; i < QSFP_DD_MAX_CHANNELS; ++i) {
		snprintf(field_desc, QSFP_DD_MAX_DESC_SIZE, cs,
			 "Rx input optical power", i + 1);
		PRINT_xX_PWR(field_desc, sd.scd[i].rx_power);
	}

	/* Print the Rx alarms/warnings for each channel */
	for (i = 0; i < QSFP_DD_MAX_CHANNELS; ++i) {
		for (j = 0; j < 4; ++j) {
			snprintf(field_desc, QSFP_DD_MAX_DESC_SIZE,
				 aw_strings[j], "Rx", i + 1);
			printf("\t%-41s : %s\n", field_desc,
			       ONOFF(sd.rxaw[i][j]));
		}
	}

	/* Print the Tx alarms/warnings for each channel */
	for (i = 0; i < QSFP_DD_MAX_CHANNELS; ++i) {
		for (j = 0; j < 4; ++j) {
			snprintf(field_desc, QSFP_DD_MAX_DESC_SIZE,
				 aw_strings[j], "Tx", i + 1);
			printf("\t%-41s : %s\n", field_desc,
			       ONOFF(sd.rxaw[i][j]));
		}
	}

	qsfp_dd_show_thresholds(sd);
}

/**
 * Print relevant info about the maximum supported fiber media length
 * for each type of fiber media at the maximum module-supported bit rate.
 * Relevant documents:
 * [1] CMIS Rev. 3, page 64, section 1.7.4.2, Table 39
 * [2] CMIS Rev. 4, page 99, section 8.4.2, Table 8-27
 */
static void qsfp_dd_show_link_len(const __u8 *id)
{
	qsfp_dd_print_smf_cbl_len(id);
	sff_show_value_with_unit(id, QSFP_DD_OM5_LEN_OFFSET,
				 "Length (OM5)", 2, "m");
	sff_show_value_with_unit(id, QSFP_DD_OM4_LEN_OFFSET,
				 "Length (OM4)", 2, "m");
	sff_show_value_with_unit(id, QSFP_DD_OM3_LEN_OFFSET,
				 "Length (OM3 50/125um)", 2, "m");
	sff_show_value_with_unit(id, QSFP_DD_OM2_LEN_OFFSET,
				 "Length (OM2 50/125um)", 1, "m");
}

/**
 * Show relevant information about the vendor. Relevant documents:
 * [1] CMIS Rev. 3, page 56, section 1.7.3, Table 27
 * [2] CMIS Rev. 4, page 91, section 8.2, Table 8-15
 */
static void qsfp_dd_show_vendor_info(const __u8 *id)
{
	sff_show_ascii(id, QSFP_DD_VENDOR_NAME_START_OFFSET,
		       QSFP_DD_VENDOR_NAME_END_OFFSET, "Vendor name");
	qsfp_dd_show_oui(id);
	sff_show_ascii(id, QSFP_DD_VENDOR_PN_START_OFFSET,
		       QSFP_DD_VENDOR_PN_END_OFFSET, "Vendor PN");
	sff_show_ascii(id, QSFP_DD_VENDOR_REV_START_OFFSET,
		       QSFP_DD_VENDOR_REV_END_OFFSET, "Vendor rev");
	sff_show_ascii(id, QSFP_DD_VENDOR_SN_START_OFFSET,
		       QSFP_DD_VENDOR_SN_END_OFFSET, "Vendor SN");
	sff_show_ascii(id, QSFP_DD_DATE_YEAR_OFFSET,
		       QSFP_DD_DATE_VENDOR_LOT_OFFSET + 1, "Date code");

	if (id[QSFP_DD_CLEI_PRESENT_BYTE] & QSFP_DD_CLEI_PRESENT_MASK)
		sff_show_ascii(id, QSFP_DD_CLEI_START_OFFSET,
			       QSFP_DD_CLEI_END_OFFSET, "CLEI code");
}

void qsfp_dd_show_all(const __u8 *id, __u32 eeprom_len)
{
	qsfp_dd_show_identifier(id);
	qsfp_dd_show_power_info(id);
	qsfp_dd_show_connector(id);
	qsfp_dd_show_cbl_asm_len(id);
	qsfp_dd_show_sig_integrity(id);
	qsfp_dd_show_mit_compliance(id);
	qsfp_dd_show_sig_optical_pwr(id, eeprom_len);
	qsfp_dd_show_link_len(id);
	qsfp_dd_show_vendor_info(id);
	qsfp_dd_show_rev_compliance(id);
}
