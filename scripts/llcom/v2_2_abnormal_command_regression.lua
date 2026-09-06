-- My_PID_Motor v2.2 abnormal-command regression
-- Firmware branch: v2.2-param-persist-and-abnormal-regression
-- Firmware commit: ee1f51d
-- LLCom serial settings: 115200, 8N1, text mode, CRLF
-- The script creates a timestamped TX/RX log under the project logs directory.

local COMMAND_WAIT_MS = 500
local STATUS_WAIT_MS = 1200
local LOG_PATH = "F:/hal_stm32_project/My_PID_Motor/logs/v2.2_abnormal_command_regression_raw_" ..
    os.date("%Y%m%d_%H%M%S") .. ".txt"

-- Create the raw log before sending any test command.
local raw_log, log_error = io.open(LOG_PATH, "wb")
if raw_log == nil then
    log.info("LOG_OPEN_FAIL", LOG_PATH, log_error)
    error("Cannot open test log: " .. tostring(log_error))
end

raw_log:write("LLCom v2.2 abnormal-command regression\r\n")
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

-- Send one invalid case and print the exact expected response.
local function runInvalidCase(command, expected)
    writeMark("CASE", command .. " => " .. expected)
    if not requireCommand(command, COMMAND_WAIT_MS) then
        return false
    end
    return true
end

sys.taskInit(function()
    writeMark("TEST_START", "v2.2 abnormal command regression")
    writeMark("FIRMWARE", "branch=v2.2-param-persist-and-abnormal-regression, commit=ee1f51d")

    -- Build the stopped UART-source baseline that must survive all invalid inputs.
    writeMark("CASE", "baseline")
    if not requireCommand("stop") then return end
    if not requireCommand("set target uart") then return end
    if not requireCommand("kp=0.05") then return end
    if not requireCommand("ki=0") then return end
    if not requireCommand("kd=0") then return end
    if not requireCommand("t=600") then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "enable:0,State:IDLE,source:Uart,target:600,PWM:0,fault:NONE,kp:0.050,ki:0.000,kd:0.000")

    -- Send the eight frozen abnormal-command cases.
    if not runInvalidCase("t=1001", "ERR:OUT_OF_RANGE") then return end
    if not runInvalidCase("t=-1001", "ERR:OUT_OF_RANGE") then return end
    if not runInvalidCase("t=499", "ERR:OUT_OF_RANGE") then return end
    if not runInvalidCase("t=-499", "ERR:OUT_OF_RANGE") then return end
    if not runInvalidCase("t=abc", "ERR:BAD_INT") then return end
    if not runInvalidCase("kp=-0.01", "ERR:OUT_OF_RANGE") then return end
    if not runInvalidCase("kp=100.01", "ERR:OUT_OF_RANGE") then return end
    if not runInvalidCase("STATUS", "ERR:BAD_CMD") then return end

    -- Confirm the invalid inputs did not change the stopped baseline.
    writeMark("CASE", "final baseline")
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "enable:0,State:IDLE,source:Uart,target:600,PWM:0,fault:NONE,kp:0.050,ki:0.000,kd:0.000")

    writeMark("TEST_DONE", "Verify all eight ERR responses with no unexpected OK response.")
    closeLog()
end)
