ra-b6010-48gt4x-r0.dtb: ra-b6010-48gt4x-r0.dts ctc5236.dtsi ctc5236-clock.dtsi
	cpp -nostdinc -I. -undef -x assembler-with-cpp ra-b6010-48gt4x-r0.dts > tmp.dts
	dtc -O dtb -o ra-b6010-48gt4x-r0.dtb tmp.dts
	rm tmp.dts -rf
	