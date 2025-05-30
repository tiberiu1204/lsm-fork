rule ExecShellcode
{
    strings:
        $shellcode = { 90 90 90 90 }
    condition:
        $shellcode
}
rule ContainsVirusString
{
    strings:
        $a = "virus"

    condition:
        $a
}