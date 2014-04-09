require('luaunit')

local StringLibraryExtensions = {}

	function StringLibraryExtensions:make_map(v)
		local m = table.create and table.create(0, #v) or {}
		for i = 1, #v do
			m[v[i]] = v[i]
		end
		return m
	end

	function StringLibraryExtensions:TestSplitWithSingleSeparator()
		local split = string.split
		assertError(split, nil, '')
		assertError(split, '', nil)
		assertError(split, nil, nil)
		assertEquals(split('',      '' ), { ''            })
		assertEquals(split('',      ','), { ''            })
		assertEquals(split('a,b',   '' ), { 'a,b'         })
		assertEquals(split('a,b',   ','), { 'a', 'b'      })
		assertEquals(split('a,b,c', ','), { 'a', 'b', 'c' })
		assertEquals(split(',a,b',  ','), { '', 'a', 'b'  })
		assertEquals(split('a,b,',  ','), { 'a', 'b', ''  })
		assertEquals(split('a,,b',  ','), { 'a', '', 'b'  })
	end

	function StringLibraryExtensions:TestSplitWithMultipleSeparators()
		local split = string.split
		assertEquals(split('',      '.,;'), { ''            })
		assertEquals(split('a.b,c', '.,;'), { 'a', 'b', 'c' })
		assertEquals(split('a.b,c', ',;' ), { 'a.b', 'c'    })
	end

	function StringLibraryExtensions:TestJoin()
		local join = string.join
		assertError(join, nil, nil)
		assertError(join, nil, '')
		assertError(join, '', nil)
		assertError(join, '')
		assertEquals(join('',   ''           ), '')
		assertEquals(join(',',  ''           ), '')
		assertEquals(join(',',  'a'          ), 'a')
		assertEquals(join(',',  'a', 'b'     ), 'a,b')
		assertEquals(join(',',  'a', 'b', 'c'), 'a,b,c')
		assertEquals(join('.,', 'a', 'b'     ), 'a.,b')
	end

	function StringLibraryExtensions:TestJoinVector()
		local joinv = string.joinv
		assertError(joinv, nil, nil)
		assertError(joinv, nil, '')
		assertError(joinv, '', nil)
		assertError(joinv, '')
		assertEquals(joinv('',   {               }), '')
		assertEquals(joinv(',',  {               }), '')
		assertEquals(joinv(',',  { ''            }), '')
		assertEquals(joinv(',',  { '', ''        }), ',')
		assertEquals(joinv(',',  { 'a', 'b'      }), 'a,b')
		assertEquals(joinv('.,', { 'a', 'b', 'c' }), 'a.,b.,c')
	end

	function StringLibraryExtensions:TestJoinHashTable()
		local joinh = string.joinh
		assertError(joinh, nil, nil)
		assertError(joinh, nil, '')
		assertError(joinh, '', nil)
		assertError(joinh, '')
		assertEquals(joinh('',  {}), '')
		assertEquals(joinh(',', {}), '')
		assertEquals(joinh(',',  self:make_map({ ''            })), '')
		assertEquals(joinh(',',  self:make_map({ '', ''        })), ',')
		assertEquals(joinh(',',  self:make_map({ 'a', 'b'      })), 'a,b')
		assertEquals(joinh('.,', self:make_map({ 'a', 'b', 'c' })), 'a.,b.,c')
	end

	function StringLibraryExtensions:TestTrim()
		local trim = string.trim
		assertError(trim, nil)
		assertEquals(trim(''                    ), '')
		assertEquals(trim('   '                 ), '')
		assertEquals(trim('12'                  ), '12')
		assertEquals(trim(' 12 '                ), '12')
		assertEquals(trim('  1 2  '             ), '1 2')
		assertEquals(trim('\r\n\t\f 1\r\n\t\f\ '), '1')
		assertEquals(trim('\0'                  ), '')
	end

	function StringLibraryExtensions:TestLeftTrim()
		local ltrim = string.ltrim
		assertError(ltrim, nil)
		assertEquals(ltrim(''                    ), '')
		assertEquals(ltrim('   '                 ), '')
		assertEquals(ltrim('12'                  ), '12')
		assertEquals(ltrim(' 12 '                ), '12 ')
		assertEquals(ltrim('  1 2  '             ), '1 2  ')
		assertEquals(ltrim('\r\n\t\f 1\r\n\t\f\ '), '1\r\n\t\f ')
		assertEquals(ltrim('\0'                  ), '')
	end

	function StringLibraryExtensions:TestRightTrim()
		local rtrim = string.rtrim
		assertError(rtrim, nil)
		assertEquals(rtrim(''                    ), '')
		assertEquals(rtrim('   '                 ), '')
		assertEquals(rtrim('12'                  ), '12')
		assertEquals(rtrim(' 12 '                ), ' 12')
		assertEquals(rtrim('  1 2  '             ), '  1 2')
		assertEquals(rtrim('\r\n\t\f 1\r\n\t\f\ '), '\r\n\t\f\ 1')
		assertEquals(rtrim('\0'                  ), '')
	end

LuaUnit:run(StringLibraryExtensions)
