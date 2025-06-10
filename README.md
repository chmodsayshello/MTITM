# MTITM - A Man in the Middle Attack on Minetest/Luanti

This program pretends to be a Luanti server, yet actually changes any user's password on a totally different server upon them connecting. This has been made for the sake of demonstrating how vulnerable the custom UDP-Protocl is to such attacks, and use for anything else is highly discouraged.

# Flaws
Like stated above, this program has been made for the sole purpose of demonstrating design weaknesses of the Minetest Network Protocol. Therefore, this has not been made to actually perform attacks in the real world, which is why this program has several issues, some theoretically being "fixable", others by design not.

 - Only one peer at a time
 - Invalid client responses lead to the program to idle indefinetly or terminate
 - Users need to use the same password on "our" and the actual "remote" server


# Thanks
Thanks all contributors to the following projects, which have made this software possible:

 - [Mini-GMP](https://github.com/chfast/mini-gmp)
 - [csrp-gmp](https://github.com/chfast/mini-gmp)

