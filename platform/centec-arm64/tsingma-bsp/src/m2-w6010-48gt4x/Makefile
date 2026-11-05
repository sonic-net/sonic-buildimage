m2-w6010-48gt4x-r0.dtb: m2-w6010-48gt4x-r0.dts ctc5236.dtsi ctc5236-clock.dtsi
	cpp -nostdinc -I. -undef -x assembler-with-cpp m2-w6010-48gt4x-r0.dts > tmp.dts
	dtc -O dtb -o m2-w6010-48gt4x-r0.dtb tmp.dts
	rm tmp.dts -rf
	
