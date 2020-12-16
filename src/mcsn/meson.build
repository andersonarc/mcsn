# main project

# prepare build files
mcsn_include = include_directories('include')
mcsn_src = files('main.c', 'mcsn.c')

# compile executable
executable('mcsn', [mcsn_src, protocol[1], protocol[2]],
    include_directories: [mcsn_include, mcpacket_include, csafe_include],
    link_with: mcpacket)