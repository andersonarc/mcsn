# main project

# prepare build files
mcsn_include = include_directories('include')
mcsn_src = files('main.c')

# compile executable
executable('mcsn', mcsn_src, 
    include_directories: [mcpacket_include, mcsn_include], 
    link_with: mcpacket)