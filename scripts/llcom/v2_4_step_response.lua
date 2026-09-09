-- My_PID_Motor v2.4.1 step-response and steady-state baseline
-- LLCom serial settings: 115200, 8N1, text mode, CRLF
-- Safety: fix the motor, keep it unloaded, and keep power-off available.
-- This script records raw status lines; it does not calculate PASS/FAIL automatically.

local COMMAND_WAIT_MS = 400
local STATUS_WAIT_MS = 1000
local SAMPLE_WAIT_MS = 100
local BASELINE_MS = 5000
local STEP_UP_MS = 5000
local STEP_DOWN_MS = 5000
local LOG_PATH = "F:/hal_stm32_project/My_PID_Motor/logs/v2.4_step_response_raw_" ..
    os.date("%Y%m%d_%H%M%S") .. ".txt"

local raw_log, log_error = io.open(LOG_PATH, "wb")
if raw_log == nil then
    log.info("LOG_OPEN_FAIL", LOG_PATH, log_error)
    error("Cannot open test log: " .. tostring(log_error))
end

raw_log:write("LLCom v2.4.1 step response and steady-state baseline\r\n")
raw_log:write("firmware_branch=v2.4\r\n")
raw_log:write("firmware_commit=f49ecb5\r\n")
raw_log:write("baseline_pid=kp=0.05,ki=0,kd=0\r\n")
raw_log:write("serial=115200,8N1,CRLF\r\n")
raw_log:write("sample_interval_ms=", tostring(SAMPLE_WAIT_MS), "\r\n")
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
    local delay_ms = wait_ms or COMMAND_WAIT_MS
    if delay_ms <= 0 then
        delay_ms = 1
    end
    sys.wait(delay_ms)
    return true
end

local function stopAndClose(reason)
    writeMark("SCRIPT_ABORT", reason)
    apiSend("uart", "stop\r\n")
    writeTrace("TX", "stop\r\n")
    closeLog()
end

local function requireCommand(command, wait_ms)
    if sendCommand(command, wait_ms) then
        return true
    end
    stopAndClose("Send failed; stop was attempted.")
    return false
end

local function sampleStatusPhase(name, target, duration_ms)
    writeMark("PHASE_START", name .. ",target=" .. tostring(target) .. ",duration_ms=" .. tostring(duration_ms))
    local elapsed = 0
    while elapsed < duration_ms do
        if not sendCommand("status", 20) then
            stopAndClose("Status sampling failed; stop was attempted.")
            return false
        end
        sys.wait(SAMPLE_WAIT_MS)
        elapsed = elapsed + SAMPLE_WAIT_MS
    end
    writeMark("PHASE_END", name)
    return true
end

local function runTest()
    writeMark("TEST_START", "v2.4.1 step response and steady-state baseline")
    writeMark("PURPOSE", "Measure steady-state error, overshoot, rise/settling behavior and baseline response.")
    writeMark("SAFETY", "Stop and remove power immediately if noise, runaway, heat or fault appears.")

    if not requireCommand("stop") then return end
    if not requireCommand("set target uart") then return end
    if not requireCommand("kp=0.05") then return end
    if not requireCommand("ki=0") then return end
    if not requireCommand("kd=0") then return end
    if not requireCommand("t=500") then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end

    writeMark("CASE", "baseline target=500")
    if not requireCommand("run=1", 2000) then return end
    if not sampleStatusPhase("BASELINE_500", 500, BASELINE_MS) then return end

    writeMark("CASE", "step target 500->800")
    if not requireCommand("t=800") then return end
    if not sampleStatusPhase("STEP_UP_800", 800, STEP_UP_MS) then return end

    writeMark("CASE", "step target 800->500")
    if not requireCommand("t=500") then return end
    if not sampleStatusPhase("STEP_DOWN_500", 500, STEP_DOWN_MS) then return end

    writeMark("CASE", "stop and final checks")
    if not requireCommand("stop", 700) then return end
    if not requireCommand("status", STATUS_WAIT_MS) then return end
    if not requireCommand("get fault", STATUS_WAIT_MS) then return end
    writeMark("EXPECT", "Final state should be enable=0, State=IDLE, PWM=0, fault=NONE.")
    writeMark("ANALYSIS", "Use firmware ms fields to calculate steady error, overshoot, rise time and settling time.")
    writeMark("TEST_DONE", "Review phase markers and all raw status lines; no automatic PASS/FAIL was applied.")
    closeLog()
end

sys.taskInit(function()
    local ok, err = pcall(runTest)
    if not ok then
        stopAndClose("Lua runtime error: " .. tostring(err))
    end
end)
