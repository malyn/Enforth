# Break inside of go and then type 'vmvars' to start the auto-display.
define vmvars
	display this->dataStack[28]
	display this->dataStack[29]
	display this->dataStack[30]
	display tos
	display &this->dataStack[32] - restDataStack
	display/x op
end
