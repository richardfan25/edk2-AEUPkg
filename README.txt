# There are three applications in the AdvEc package,
# 1. ecutil	- ITE EC tool
# 2. rdcutil	- RDC EC tool
# 3. rdccm	- RDC EC communication tool

# The example directory is shown below as ecutil.

### Build For EFI with EDK II
### Note. The parameter "-a X64" is mean that the output file is working on x64 arch 
$(EDK_WORKSPACE)\build -p $(EDK_WORKSPACE)\AdvEcPkg\Adv_ECPkg.dsc -a X64
OUTPUT dir: $(EDK_WORKSPACE)\build\AdvEc\RELEASE_$(CC)\$(ARCH)\ECutil.efi

### Build For Linux
cd $(EDK_WORKSPACE)\AdvEcPkg\Applications\ecutil
make all
OUTPUT dir: .\