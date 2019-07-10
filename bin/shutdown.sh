# Shutdown sequence 
# Command Sequence is DISABLE > STANDBY > EXIT_CONTROL 

python3 command.py -D ATArchiver disable
echo "[OK] DISABLE COMPLETE." 
sleep 4

python3 command.py -D ATArchiver standby
echo "[OK] STANDBY COMPLETE." 
sleep 4 

python3 command.py -D ATArchiver exitControl
echo "[OK] EXIT_CONTROL  COMPLETE." 
sleep 4
