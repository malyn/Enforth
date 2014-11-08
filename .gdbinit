# Break inside of go and then type 'vmvars' to start the auto-display.
define vmvars
	display *(EnforthCell*)(vm->cur_task.ram + (32+128+96) - 16)
	display *(EnforthCell*)(vm->cur_task.ram + (32+128+96) - 12)
	display *(EnforthCell*)(vm->cur_task.ram + (32+128+96) - 8)
	display tos
	display (EnforthCell*)(vm->cur_task.ram + (32+128+96)) - restDataStack
	display w
	display (enum EnforthToken)token
end
