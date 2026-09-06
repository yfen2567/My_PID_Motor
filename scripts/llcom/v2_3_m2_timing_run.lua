-- My_PID_Motor v2.3.1 M2 running timing test
-- LLCom serial settings: 115200, 8N1, text mode, CRLF
-- Safety: fix the motor, keep it unloaded, keep power-off available.
-- The script creates a timestamped TX/RX log under the project logs directory.
-- This script does not decide PASS/FAIL automatically; stop immediately on unsafe behavior.

local COMMAND_WAIT_MS = 300
local STATUS_WAIT_MS = 1000
local RUN_SETTLE_MS = 2000
local SAMPLE_COUNT = 20
local LOG_PATH = "F:/hal_stm32_project/My_PID_Motor/logs/v2.3_m2_timing_run_raw_" ..
    os.date("%Y%m%d_%H%M%S") .. ".txt"

local raw_log, log_error = io.open(LOG_PATH, "wb")
if raw_log == nil then
    log.info("LOG_OPEN_FAIL", LOG_PATH, log_error)
    error("Cannot open test log: " .. tostring(log_error))
end

raw_log:write("LLCom v2.3.1 M2 running timing test\r\n")
raw_log:write("firmware_branch=v2.3.1-timing-observability\r\n")
raw_log:write("firmware_commit=3aa18c2\r\n")
raw_log:write("working_tree=uncommitted v2.3 timing changes; verify before release\r\n")
raw_log:write("serial=115200,8N1,CRLF\r\n")
raw_log:write("target=600,source=UART\r\n")
raw_log:write("sample_count=", tostring(SAMPLE_COUNT), "\r\n")
raw_log:write("script_log=", LOG_PATH, "\r\n\r\n")
raw_log:flush()

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

uartReceive = function(data)
    log.info("RX", data)
    writeTrace("RX", data)
end

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

local function abortWithStop(reason)
    writeMark("SCRIPT_ABORT", reason)
    apiSend("uart", "stop\r\n")
    writeTrace("TX", "stop\r\n")
    closeLog()
end

local function requireCommand(command, wait_ms)
    if sendCommand(command, wait_ms) then
        return true
    end
    abortWithStop("Send failed; stop was attempted.")
    return false
end

sys.taskInit(function()
    writeMark("TEST_START", "M2 running timing")
    writeMark("PURPOSE", "Measure timing while the closed-loop motor control is running.")
    writeMark("SAFETY", "Stop and remove power immediately if noise, runaway, heat or fault appears.")

    if not requireCommand("stop") then return end
    if not requireCommand("set target uart") then return end
    if not requireCommand("t=600") then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end

    writeMark("CASE", "run and settle")
    if not requireCommand("run=1", RUN_SETTLE_MS) then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "enable=1, State=RUN, source=Uart, target=600, fault=NONE.")

    for index = 1, SAMPLE_COUNT do
        writeMark("SAMPLE", tostring(index))
        if not requireCommand("timing stats", COMMAND_WAIT_MS) then return end
    end

    if not requireCommand("comm stats", STATUS_WAIT_MS) then return end
    writeMark("CASE", "stop and settle")
    if not requireCommand("stop", 700) then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "enable=0, State=IDLE, PWM=0, fault=NONE after stop.")
    writeMark("TEST_DONE", "Review running samples, final status and COMM_STATS.")
    closeLog()
end)
