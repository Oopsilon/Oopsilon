
function myfib(number)
	if  number > 1 then
		return myfib(number - 1) + myfib(number - 2)
	else
		return number
	end
end

myfib(40)
