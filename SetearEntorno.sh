pathTP='/home/utnso'

echo "######################## TP DELIBIRD - Ripped Dinos ##########################"
echo "Compilando Shared Library..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib/Debug/"
make all

echo "Compilando broker..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Broker/Debug/"
make all

echo "Compilando team..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Team/Debug/"
make all

echo "Compilando gamecard..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Game-Card/Debug/"
make all

echo "Compilando gameboy..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Game-Boy/Debug/"
make all
