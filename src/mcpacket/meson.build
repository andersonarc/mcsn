# packet library

# get programs
python = find_program('python')

# generate build files
run_command(python, ['-m', 'pip', 'install', '-r', '@INPUT@'])
protocol = custom_target('protocol', 
    build_by_default: true,
    input: 'mcd2packet/mcd2packet.py', 
    output: ['protocol.c', 'protocol.h', 'particle_types.h'],
    command: [python, '@INPUT@'])

# prepare build files
mcpacket_src = files('data_utils.c', 'translation_utils.c')
mcpacket_include = include_directories('include')

# compile library
mcpacket = library('mcpacket', [mcpacket_src, protocol], 
    include_directories: mcpacket_include)