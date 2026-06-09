erl = find_program('erl', required : get_option('erlang_bindings'))
erlc = find_program('erlc', required : get_option('erlang_bindings'))

if erl.found() and erlc.found()
    cc = meson.get_compiler('c')
    erl_root_cmd = run_command(
        erl,
        '-noshell',
        '-eval',
        'io:format("~s", [code:root_dir()]), halt().',
        check : true,
    )
    erl_root = erl_root_cmd.stdout()
    erl_include = include_directories(erl_root / 'usr/include')
    have_erl_nif = cc.has_header(
        'erl_nif.h',
        include_directories : erl_include,
        required : get_option('erlang_bindings'),
    )

    if have_erl_nif
        erlang_ebin_dir = meson.current_build_dir()

        wrouter_nif = shared_module(
            'wrouter_nif',
            'c_src/wrouter_nif.c',
            include_directories : [inc, erl_include],
            dependencies : [libwrouter_static_dep],
            name_prefix : '',
            install : true,
            install_dir : get_option('libdir') / 'erlang/lib/wrouter/priv',
        )

        wrouter_beam = custom_target(
            'wrouter_beam',
            input : 'src/wrouter.erl',
            output : 'wrouter.beam',
            command : [
                erlc,
                '-o',
                erlang_ebin_dir,
                '@INPUT@',
            ],
            build_by_default : true,
            install : true,
            install_dir : get_option('libdir') / 'erlang/lib/wrouter/ebin',
        )

        eunit_cmd = run_command(
            erl,
            '-noshell',
            '-eval',
            'case code:lib_dir(eunit) of {error, bad_name} -> halt(1); _ -> halt(0) end.',
            check : false,
        )

        if eunit_cmd.returncode() == 0
            wrouter_tests_beam = custom_target(
                'wrouter_tests_beam',
                input : 'test/wrouter_tests.erl',
                output : 'wrouter_tests.beam',
                command : [
                    erlc,
                    '-I',
                    meson.current_source_dir() / 'src',
                    '-o',
                    erlang_ebin_dir,
                    '@INPUT@',
                ],
                build_by_default : false,
                depends : wrouter_beam,
            )

            test(
                'erlang',
                erl,
                args : [
                    '-noshell',
                    '-pa',
                    erlang_ebin_dir,
                    '-eval',
                    'case eunit:test(wrouter_tests, [verbose]) of ok -> halt(0); _ -> halt(1) end.',
                ],
                env : {
                    'WROUTER_NIF_LIB' : wrouter_nif.full_path(),
                },
                depends : [wrouter_nif, wrouter_beam, wrouter_tests_beam],
            )
        endif
    endif
endif
