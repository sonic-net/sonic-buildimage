set -e
echo "JA Setting"
echo ""

echo "Register Configuration Start"

if [ "$1" = "25PPM" ]; then
    echo "Configuring 25PPM settings ..."
    /usr/share/sonic/platform/dpllupgrade_test spi -w /usr/share/sonic/platform/z9864f_ZL30793_option-1_0811_HP-Synth1+20ppm_all.mfg
else
    echo "Configuring 0 PPM settings ..."
    /usr/share/sonic/platform/dpllupgrade_test spi -w /usr/share/sonic/platform/z9864f_ZL30793_option-1_0811.mfg
fi

echo "Register Configuration End"