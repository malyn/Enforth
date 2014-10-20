# Break inside of go and then type 'vmvars' to start the auto-display.
define vmvars
	display vm->data_stack[28]
	display vm->data_stack[29]
	display vm->data_stack[30]
	display tos
	display &vm->data_stack[32] - restDataStack
	display w
	display (enum EnforthToken)token
end
