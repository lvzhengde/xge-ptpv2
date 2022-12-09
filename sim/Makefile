elfFile = ptpv2
waveFile = ptpv2

tc_rtc:
	iverilog -o $(elfFile).out -s tc_rtc -f comp.f  

vsim:
	vvp -n $(elfFile).out -fst

debug:
	gtkwave $(waveFile).fst

clean:
	rm -rf *.fst *.lxt *.vcd *.out
