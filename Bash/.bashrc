# Fancy 2-line bash prompt (Fixed Hostname fallback)
PS1="\[\e[36m\]┌─[\[\e[35m\]\u\[\e[37m\]@\[\e[32m\]\${HOSTNAME:-{\$(hostname)}}\[\e[36m\]]─[\[\e[33m\]\w\[\e[36m\]]\n\[\e[36m\]└─\[\e[35m\]❯ \[\e[0m\]"
