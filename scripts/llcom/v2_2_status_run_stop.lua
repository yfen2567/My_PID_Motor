-- My_PID_Motor v2.2 status/run/stop/restart regression
-- Firmware branch: v2.2-param-persist-and-abnormal-regression
-- Firmware commit: ee1f51d
-- LLCom serial settings: 115200, 8N1, text mode, CRLF
-- The script creates a timestamped TX/RX log under the project logs directory.
-- Safety: fix the motor, keep it unloaded, and keep power-off available.

local COMMAND_WAIT_MS = 500
local STATUS_WAIT_MS = 1200
local MOTOR_RUN_MS = 2000
local STOP_SETTLE_MS = 500
local LOG_PATH = "F:/hal_stm32_project/My_PID_Motor/logs/v2.2_status_run_stop_raw_" ..
    os.date("%Y%m%d_%H%M%S") .. ".txt"

-- Create the raw log before sending any test command.
local raw_log, log_error = io.open(LOG_PATH, "wb")
if raw_log == nil then
    log.info("LOG_OPEN_FAIL", LOG_PATH, log_error)
    error("Cannot open test log: " .. tostring(log_error))
end

raw_log:write("LLCom v2.2 status/run/stop/restart regression\r\n")
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

-- Attempt a stop before aborting after any LLCom send failure.
local function requireCommand(command, wait_ms)
    if sendCommand(command, wait_ms) then
        return true
    end

    writeMark("SCRIPT_ABORT", "Attempting stop. Cut motor power if it is still running.")
    apiSend("uart", "stop\r\n")
    writeTrace("TX", "stop\r\n")
    closeLog()
    return false
end

sys.taskInit(function()
    writeMark("TEST_START", "v2.2 status/run/stop/restart")
    writeMark("FIRMWARE", "branch=v2.2-param-persist-and-abnormal-regression, commit=ee1f51d")

    -- Establish the fixed UART-source, target=600 baseline while stopped.
    writeMark("CASE", "stopped baseline")
    if not requireCommand("stop") then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    if not requireCommand("set target uart") then return end
    if not requireCommand("kp=0.05") then return end
    if not requireCommand("ki=0") then return end
    if not requireCommand("kd=0") then return end
    if not requireCommand("t=600") then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "source:Uart,target:600,enable:0,State:IDLE,PWM:0,fault:NONE")

    -- First run and stop cycle.
    writeMark("CASE", "first run")
    if not requireCommand("run=1", MOTOR_RUN_MS) then return end
    writeMark("EXPECT", "OK:RUN")
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "enable:1,State:RUN,source:Uart,target:600,fault:NONE")

    writeMark("CASE", "first stop")
    if not requireCommand("stop", STOP_SETTLE_MS) then return end
    writeMark("EXPECT", "OK:STOPPED")
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "enable:0,State:IDLE,PWM:0")

    -- Second run and stop cycle proves restart after a prior stop.
    writeMark("CASE", "second run")
    if not requireCommand("run=1", MOTOR_RUN_MS) then return end
    writeMark("EXPECT", "OK:RUN")
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "enable:1,State:RUN,source:Uart,target:600,fault:NONE")

    writeMark("CASE", "final stop")
    if not requireCommand("stop", STOP_SETTLE_MS) then return end
    writeMark("EXPECT", "OK:STOPPED")
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "enable:0,State:IDLE,PWM:0")
    if not requireCommand("get fault", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "FAULT_SNAPSHOT_NONE")

    writeMark("TEST_DONE", "Check every stated run/stop status transition.")
    closeLog()
end)
