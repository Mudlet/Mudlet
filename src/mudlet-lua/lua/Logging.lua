----------------------------------------------------------------------------------
--- Mudlet Logging
----------------------------------------------------------------------------------
-- <pre>
-- The basic design of this system is inspired by Python's logging module, which is
-- in turn inspired by log4J. However it does diverge from both.
--
-- The system creates a hierarchy of Loggers. A logger is the interface through which
-- code sends messages into the logging system. These messages can be simple strings
-- (commonly), or tables. In either case, the logger converts the message into an
-- event which is a table with certain standard keys, and a number of custom keys
-- that a logger can provide if it wishes.
--
-- Events have a number of standard properties: a timestamp, the name of the logger,
-- a severity(level) of the event, and the message itself.
--
-- By default, a logger has a level of ANY, which means that any events passed into
-- it are handled. You can set this level higher if you want, in which case the
-- logger will ignore any events which are of a lesser severity.
--
-- The level of severities are:
--   ANY, DEBUG, INFO, WARNING, ERROR, CRITICAL
--
-- A logger can have any number of handlers assigned to it.
--
-- A handler is responsible for actually writing/saving the data to the device,
-- and has a formatter associated with it.
--
-- Precisely what a "device" is depends on the type of handler; for a FileHandler,
-- its an actual filename. For a ConsoleHandler, its always just true as it writes
-- to the main console. For a MiniConsoleHandler, its the name of a miniconsole.
-- For a SheetHandler, its a db frontend sheet reference, e.g:
--    local mydb = db:get_database("tracker")
--    local device = mydb.kills
--
-- Each handler can also have its own severity, which means it will ignore any
-- events that are below that. This is useful if you want a logger to write only
-- important messages to the screen, but write more to a file.
--
-- Every logger has a "propagate" property which defaults to true. When true,
-- any events it handles will propagate to the parent logger. Its important to note
-- that a single event will only ever be written to a certain device once, so its
-- safe to propagate up the chain even if a parent logger would write things out
-- too. Whatever handler is assigned to the event first for a given device, wins.
-- </pre>


logging = {
	-- Formatters are used to convert a logging event into a form which is appropriate
    -- for a certain kind of output device. Handlers actually write to such a device,
    -- but formatters are used by handlers.
	--
	-- By default, there are four kinds of output devices that are supported and thus
 	-- four default formatters that are used when outputting to devices of that type.
	-- A specific logger can override this for specific handlers, of course.
	--
	-- The file default is used when writing to the actual filesystem, and is the
	-- most verbose output format.
	--
	-- The console default is meant to write to the actual main mud window.
	--
	-- The miniconsole is meant to write to miniconsoles, which we assume may be
	-- much smaller then the main window and so we write only the message by default.
	--
	-- Finally, the sheet default is meant to be used with the handler that writes
	-- logging events to database sheets that are constructed with Mudlet's db
	-- frontend. This formatter is a table and not a string, and contains a key
	-- for each field in the table you want to add, and either a function that
	-- returns what value to write, or a string which is the key of the given
	-- event.

	_formatters = {
		default_file = "[${timestamp}] ${system} [${levelname:pad:8}]: ${message}\n",
		default_console = "[${system} ${levelname:pad:8}]: ${message}\n",
		default_miniconsole = "${message}\n",
		default_sheet = {
			timestamp = os.time,
         system="system",
		   level="levelname",
			message="message"
		}
	},
	_timestamp = {
			display = "%Y-%m-%d %H:%M:%S",
			filesystem = "%Y-%m-%d_%H.%M.%S"
	},

	-- Levels:
	ANY = 0,
	DEBUG = 10,
	INFO = 20,
	WARNING = 30,
	ERROR = 40,
	CRITICAL = 50,

	_levelnames = {
		[0] = "ANY",
		[10] = "DEBUG",
		[20] = "INFO",
		[30] = "WARNING",
		[40] = "ERROR",
		[50] = "CRITICAL"
	}
}

-- Handler Interface

logging.Handler = {}
logging.__handlerMT = {__index=logging.Handler}

--- logging.Handler:create(name, device)
function logging.Handler:create(name, device)
	assert(name)
	assert(device)

	local handler = {_name=name, _device=device, emit=function() end}
	setmetatable(handler, logging.__handlerMT)
	return handler
end

--- logging.Handler:process(event)
function logging.Handler:process(event)
	if event.level >= self.level then
		if self.emit then
			self.emit(self, event)
		end
	end
end

logging.FileHandler = {}

--- logging.FileHandler:create(path)
function logging.FileHandler:create(path)

end

-- Logger Interface

logging.Logger = {}
logging.__loggerMT = {__index=logging.Logger}

--- logging.Logger:create(name, parent)
function logging.Logger:create(name, parent)
	local logger = {
		_name=name,
		_parent=parent,
		_children={},
		_handlers = {},

		_customizers = {},

		propagate=true,
		level=logging.ANY,
	}

	setmetatable(logger, logging.__loggerMT)
	return logger
end

--- logging.Logger:add_event_customizer(fn)
function logging.Logger:add_event_customizer(fn)
	local customizers = self._customizers
	customizers[#customizers+1] = fn
end

--- logging.Logger:clear_event_customizers()
function logging.Logger:clear_event_customizers()
	self._customizers = {}
end

--- logging.Logger:_prepare_event(event)
function logging.Logger:_prepare_event(event)
	local name_chunks = {self._name}
	if self._name ~= "mudlet" then
		parent = self._parent
		while parent._name ~= "mudlet" do
			name_chunks[#name_chunks+1] = parent._name
			parent = parent._parent
		end
	end

	event.system = table.concat(name_chunks, ".")
	event.timestamp = os.time()

	return event
end

--- logging.Logger:_customize_event(event)
function logging.Logger:_customize_event(event)
	for _, customizer in ipairs(self._customizers) do
		customizer(event)
	end

	return event
end

--- logging.Logger:_do_log(evt, extras, level)
function logging.Logger:_do_log(evt, extras, level)
	if type(evt) == "string" then
		evt = {message=evt}
	else
		assert(type(evt)=="table", "The first argument to the logging call must be a string or table.")
		assert(evt.message, "If passing a table to the logging call, it must contain a message value.")
	end

	assert(level)
	evt.level = level

	if extras then
		for k, v in pairs(extras) do
			evt[k] = v
		end
	end

	self:_log_event(self:_prepare_event(evt))
end

--- logging.Logger:_log_event(evt)
function logging.Logger:_log_event(evt)
	local event = self._customize_event(evt)

	if event.level >= self.level	 then
		for _, handler in self._handlers do
			handler:process(event)
		end
	end

	if self.propagate then
		self._parent:_log_event(event)
	end
end

--- logging.Logger:debug(event, extras)
function logging.Logger:debug(event, extras)
	return self:_do_log(event, extras, logging.DEBUG)
end

--- logging.Logger:info(event, extras)
function logging.Logger:info(event, extras)
	return self:_do_log(event, extras, logging.INFO)
end

--- logging.Logger:warning(event, extras)
function logging.Logger:warning(event, extras)
	return self:_do_log(event, extras, logging.WARNING)
end

--- logging.Logger:error(event, extras)
function logging.Logger:error(event, extras)
	return self:_do_log(event, extras, logging.ERROR)
end

--- logging.Logger:critical(event, extras)
function logging.Logger:critical(event, extras)
	return self:_do_log(event, extras, logging.CRITICAL)
end

-- General Logging System

logging._root = logging.Logger:create("mudlet", nil)

--- logging:get_logger(name)
function logging:get_logger(name)
	local name = name or "mudlet"
	local loggers = name:split(".")
	local parent = logging._root

	for _, logger in ipairs(loggers) do
		if parent._children[logger] == nil then
			local child = logging.Logger:create(logger, parent)
			parent._children[logger] = child
			parent = child
		else
			parent = parent._children[logger]
		end
	end

	return parent
end

--- logging:format_event(event, formatter)
function logging:format_event(event, formatter)
	local format = formatter or logging._formatters.default_file

	local pattern = "(\${(.-)})"
	local output = format:gsub(pattern,
		function(complete, chunk)
			local compound = {chunk:match("([^:]+):([^:]+):(.+)")}
			if table.is_empty(compound) then
				return event[chunk] or complete
			else
				local value = event[compound[1]] or complete
				if compound[2] == "pad" then
					return string.format("%"..compound[3].."s", value)
				else
					return complete
				end
			end
		end
	)
	return output
end

logging:format_event({timestamp=os.date(logging._timestamp.display, os.time()), system="crucible.core", levelname="ERROR", message="Err"})

local log = logging:get_logger("crucible.core")
local root = logging:get_logger()

log:add_event_customizer(
	function(evt)
		evt["test"] = 1
	end
)
