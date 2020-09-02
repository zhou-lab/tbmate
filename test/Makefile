test_stringd:
	../tbmate pack -s stringd small/string.bed small/string.tbk
	../tbmate header small/string.tbk
	../tbmate view -o small/view_string.out small/string.tbk
	diff small/view_string.out small/string.bed
	../tbmate view -ko small/view_string2.out small/string.tbk
	diff small/view_string2.out small/string.bed

test_stringf:
	../tbmate pack -s stringf small/string.bed small/string_fixed.tbk
	../tbmate header small/string_fixed.tbk
	../tbmate view -o small/view_string_fixed.out small/string_fixed.tbk
	diff small/view_string_fixed.out small/string.bed
	../tbmate view -ko small/view_string_fixed2.out small/string_fixed.tbk
	diff small/view_string_fixed2.out small/string.bed

test_float:
	../tbmate pack -s float small/float.bed small/float.tbk
	../tbmate header small/float.tbk
	../tbmate view -o small/view_float.out small/float.tbk
	paste small/view_float.out small/float.bed | awk -f wanding.awk -e 'abs($$4-$$8)>0.001'
	../tbmate view -ko small/view_float2.out small/float.tbk
	paste small/view_float2.out small/float.bed | awk -f wanding.awk -e 'abs($$4-$$8)>0.001'

test_double:
	../tbmate pack -s double small/double.bed small/double.tbk
	../tbmate header small/double.tbk
	../tbmate view -o small/view_double.out small/double.tbk
	paste small/view_double.out small/double.bed | awk -f wanding.awk -e 'abs($$4-$$8)>0.001'
	../tbmate view -ko small/view_double2.out small/double.tbk
	paste small/view_double2.out small/double.bed | awk -f wanding.awk -e 'abs($$4-$$8)>0.001'

test_int1:
	../tbmate pack -s int1 small/int1.bed small/int1.tbk
	../tbmate header small/int1.tbk
	../tbmate view -o small/view_int1.out small/int1.tbk
	diff small/view_int1.out small/int1.bed
	../tbmate view -ko small/view_int12.out small/int1.tbk
	diff small/view_int12.out small/int1.bed

test_int2:
	../tbmate pack -s int2 small/int2.bed small/int2.tbk
	../tbmate header small/int2.tbk
	../tbmate view -o small/view_int2.out small/int2.tbk
	diff small/view_int2.out small/int2.bed
	../tbmate view -ko small/view_int22.out small/int2.tbk
	diff small/view_int22.out small/int2.bed

test_int:
	../tbmate pack -s int small/integer.bed small/integer.tbk
	../tbmate header small/integer.tbk
	../tbmate view -o small/view_integer.out small/integer.tbk
	diff small/view_integer.out small/integer.bed
	../tbmate view -ko small/view_integer2.out small/integer.tbk
	diff small/view_integer2.out small/integer.bed


clean:
	rm -f small/*.out
	rm -f small/*.tbk
