-- My_PID_Motor v2.2 parameter persistence regression
-- Firmware branch: v2.2-param-persist-and-abnormal-regression
-- Firmware commit: ee1f51d
-- LLCom serial settings: 115200, 8N1, text mode, CRLF
-- The script creates a timestamped TX/RX log under the project logs directory.

local COMMAND_WAIT_MS = 500
local STATUS_WAIT_MS = 1200
local MANUAL_RESET_WAIT_MS = 15000
local LOG_PATH = "F:/hal_stm32_project/My_PID_Motor/logs/v2.2_param_persistence_raw_" ..
    os.date("%Y%m%d_%H%M%S") .. ".txt"

-- Create the raw log before sending any test command.
local raw_log, log_error = io.open(LOG_PATH, "wb")
if raw_log == nil then
    log.info("LOG_OPEN_FAIL", LOG_PATH, log_error)
    error("Cannot open test log: " .. tostring(log_error))
end

raw_log:write("LLCom v2.2 parameter persistence regression\r\n")
raw_log:write("firmware_branch=v2.2-param-persist-and-abnormal-regression\r\n")
raw_log:write("firmware_commit=ee1f51d\r\n")
raw_log:write("serial=115200,8N1,CRLF\r\n")
raw_log:write("script_log=", LOG_PATH, "\r\n\r\n")
raw_log:flush()

-- Write TX/RX data and script markers to the project raw log.
local function writeTrace(kind, data)
    if raw_log == nil then
        return
    end

    raw_log:write("[", os.date("%Y-%m-%d %H:%M:%S"), "] ", kind, " ")
    raw_log:write(data)
    if string.sub(data, -1) ~= "\n" then
        raw_log:write("\r\n")
    end
    raw_log:flush()
end

local function writeMark(kind, data)
    log.info(kind, data)
    writeTrace(kind, data)
end

local function closeLog()
    if raw_log ~= nil then
        raw_log:flush()
        raw_log:close()
        raw_log = nil
    end
end

-- Record every received UART chunk in the LLCom Lua log.
uartReceive = function(data)
    log.info("RX", data)
    if raw_log ~= nil then
        writeTrace("RX", data)
    end
end

-- Send one command and leave enough time for the firmware response.
local function sendCommand(command, wait_ms)
    log.info("TX", command)
    writeTrace("TX", command .. "\r\n")
    if not apiSend("uart", command .. "\r\n") then
        writeMark("TX_FAIL", command)
        return false
    end

    sys.wait(wait_ms or COMMAND_WAIT_MS)
    return true
end

-- Stop the script when LLCom cannot send the next command.
local function requireCommand(command, wait_ms)
    if sendCommand(command, wait_ms) then
        return true
    end

    writeMark("SCRIPT_ABORT", "Check the serial connection and save the log.")
    closeLog()
    return false
end

sys.taskInit(function()
    writeMark("TEST_START", "v2.2 parameter persistence")
    writeMark("FIRMWARE", "branch=v2.2-param-persist-and-abnormal-regression, commit=ee1f51d")

    -- PP-001: save non-default PID gains and verify a real MCU restart reloads them.
    writeMark("CASE", "PP-001 save and reboot reload")
    if not requireCommand("stop") then return end
    if not requireCommand("set target uart") then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    if not requireCommand("kp=0.06") then return end
    if not requireCommand("ki=0.01") then return end
    if not requireCommand("kd=0.02") then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    if not requireCommand("save params", STATUS_WAIT_MS) then return end

    writeMark("EXPECT", "OK:PARAMS_SAVED")
    writeMark("MANUAL_ACTION", "Within 15 seconds, press NRST, power-cycle the MCU, or use ST-Link Reset. Do not send rst.")
    sys.wait(MANUAL_RESET_WAIT_MS)
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "kp:0.060,ki:0.010,kd:0.020, enable:0,State:IDLE,PWM:0,fault:NONE")

    -- PP-002: unsaved RAM changes must be replaced by the saved gains.
    writeMark("CASE", "PP-002 explicit load")
    if not requireCommand("stop") then return end
    if not requireCommand("kp=0.07") then return end
    if not requireCommand("ki=0.03") then return end
    if not requireCommand("kd=0.04") then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    if not requireCommand("load params", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "OK:PARAMS_LOADED")
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "kp:0.060,ki:0.010,kd:0.020")

    -- PP-003: reset params writes defaults and keeps them after a real MCU restart.
    writeMark("CASE", "PP-003 reset defaults and reboot reload")
    if not requireCommand("stop") then return end
    if not requireCommand("reset params", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "OK:PARAMS_RESET")
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "kp:0.050,ki:0.000,kd:0.000")

    writeMark("MANUAL_ACTION", "Within 15 seconds, perform a real MCU reset again. Do not send rst.")
    sys.wait(MANUAL_RESET_WAIT_MS)
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "kp:0.050,ki:0.000,kd:0.000, enable:0,State:IDLE,PWM:0,fault:NONE")

    writeMark("TEST_DONE", "Review the expected responses and reboot-time status lines.")
    closeLog()
end)
