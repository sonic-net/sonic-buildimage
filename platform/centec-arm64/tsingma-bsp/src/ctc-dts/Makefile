e530-ctc5236.dtb: e530-ctc5236.dts ctc5236.dtsi ctc5236-clock.dtsi
	cpp -nostdinc -I. -undef -x assembler-with-cpp e530-ctc5236.dts > tmp.dts
	dtc -O dtb -o e530-ctc5236.dtb tmp.dts
	rm tmp.dts -rf
