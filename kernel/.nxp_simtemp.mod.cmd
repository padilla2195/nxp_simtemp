savedcmd_nxp_simtemp.mod := printf '%s\n'   nxp_simtemp.o | awk '!x[$$0]++ { print("./"$$0) }' > nxp_simtemp.mod
