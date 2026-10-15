# A small JSON writer and reader for the records sidecar (no package: D-UX2). A record is written as an
# ordered list of pairs so that the file is the same bytes on every run.

function json_string(io::IO, s::AbstractString)
    print(io, '"')
    for c in s
        if c == '"'
            print(io, "\\\"")
        elseif c == '\\'
            print(io, "\\\\")
        elseif c == '\n'
            print(io, "\\n")
        elseif c < ' '
            print(io, "\\u", string(UInt16(c); base = 16, pad = 4))
        else
            print(io, c)
        end
    end
    print(io, '"')
end

json(io::IO, x::AbstractString) = json_string(io, x)
json(io::IO, ::Nothing) = print(io, "null")
json(io::IO, x::Bool) = print(io, x ? "true" : "false")
json(io::IO, x::Integer) = print(io, string(x))
function json(io::IO, x::AbstractVector)
    if eltype(x) <: Pair || (!isempty(x) && all(e -> e isa Pair, x))
        print(io, '{')
        for (i, (k, v)) in enumerate(x)
            i > 1 && print(io, ',')
            json_string(io, string(k)); print(io, ':'); json(io, v)
        end
        print(io, '}')
    else
        print(io, '[')
        for (i, v) in enumerate(x)
            i > 1 && print(io, ',')
            json(io, v)
        end
        print(io, ']')
    end
end
json(x) = sprint(json, x)

# Reader: objects become Dict{String,Any}, arrays Vector{Any}; numbers are integers only (the writer
# writes no other number).
mutable struct _J
    s::String
    i::Int
end
function _ws(j::_J)
    while j.i <= ncodeunits(j.s) && j.s[j.i] in (' ', '\t', '\n', '\r')
        j.i += 1
    end
end
function _jvalue(j::_J)
    _ws(j)
    c = j.s[j.i]
    if c == '{'
        j.i += 1; d = Dict{String,Any}(); _ws(j)
        if j.s[j.i] == '}'
            j.i += 1; return d
        end
        while true
            _ws(j); k = _jstring(j); _ws(j)
            j.s[j.i] == ':' || error("JSON: ':' expected at $(j.i)")
            j.i += 1
            d[k] = _jvalue(j); _ws(j)
            j.s[j.i] == ',' && (j.i += 1; continue)
            j.s[j.i] == '}' && (j.i += 1; return d)
            error("JSON: ',' or '}' expected at $(j.i)")
        end
    elseif c == '['
        j.i += 1; a = Any[]; _ws(j)
        if j.s[j.i] == ']'
            j.i += 1; return a
        end
        while true
            push!(a, _jvalue(j)); _ws(j)
            j.s[j.i] == ',' && (j.i += 1; continue)
            j.s[j.i] == ']' && (j.i += 1; return a)
            error("JSON: ',' or ']' expected at $(j.i)")
        end
    elseif c == '"'
        return _jstring(j)
    elseif startswith(SubString(j.s, j.i), "null")
        j.i += 4; return nothing
    elseif startswith(SubString(j.s, j.i), "true")
        j.i += 4; return true
    elseif startswith(SubString(j.s, j.i), "false")
        j.i += 5; return false
    else
        m = match(r"^-?[0-9]+", SubString(j.s, j.i))
        m === nothing && error("JSON: value expected at $(j.i)")
        j.i += ncodeunits(m.match)
        return parse(Int, m.match)
    end
end
function _jstring(j::_J)
    j.s[j.i] == '"' || error("JSON: string expected at $(j.i)")
    j.i += 1
    io = IOBuffer()
    while true
        c = j.s[j.i]
        if c == '"'
            j.i += 1; return String(take!(io))
        elseif c == '\\'
            e = j.s[j.i + 1]
            if e == 'u'
                print(io, Char(parse(UInt16, j.s[j.i+2:j.i+5]; base = 16))); j.i += 6
            else
                print(io, Dict('"' => '"', '\\' => '\\', '/' => '/', 'n' => '\n', 't' => '\t',
                               'r' => '\r', 'b' => '\b', 'f' => '\f')[e]); j.i += 2
            end
        else
            print(io, c); j.i = nextind(j.s, j.i)
        end
    end
end
json_parse(s::AbstractString) = _jvalue(_J(String(s), 1))
