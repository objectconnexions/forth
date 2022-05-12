
: MASK ( size pos - mask )
	swap
	1 swap lshift 1 -	\ basic mask
	swap lshift			\ move into position
\	1 3 lshift 1 -   8 lshift .
;
