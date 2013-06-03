zmq = require "zmq"
_ = require "underscore"
BSON = require "bson"
S = require "string"
msgpack = require "msgpack"
restify = require "restify"

if typeof String.prototype.startsWith != "function"
	String.prototype.startsWith 



class Response
	constructor: (@code, body, @type, @headers, @tag) ->
		#TODO: Make this more robust with try catch
		#If anything fails to parse and throws.. well
		#You'll have a bad time.
		@body = switch @type
			when "string" then body.toString()
			when "bson" then BSON.deserialize body
			when "msgpack" then msgpack.unpack body
			when "json" then JSON.parse body.toString()

class Request
	constructor: (@server, @type, @uri, @tag) ->
		console.log @server, @type, @uri

	setHeaders: (@headers) ->

	setBody: (@body) ->

	setBSONSession: (session) ->
		@version = "bson"
		@session = BSON.serialize session

	setMsgPackSession: (session) ->
		@version = "msgpack"
		@session = msgpack.pack session

	setAuth: (@auth) ->

	makeRequest: (req) ->
		#Now let's build the request.
		parts = []
		parts.push "Type:" + @type
		parts.push "URI:" + @uri
		parts.push "Tag:" + @tag

		parts.push "Auth:" + @auth if @auth
		if @version
			parts.push "Version:" + @version
		if @headers
			_.each @headers, (key, val) ->
				parts.push "Header-" + key + ":" + val
		parts.push "Body:" + @session if @session
		parts.push "Body:" + @body if @body
		
		console.log "sending", req
		req.send parts

class Requester
	constructor: (@server) ->
		@taggedRes = {}
		@tag = 0
		@req = zmq.socket "req"
		@req.connect @server
		taggedRes = @taggedRes
		parseReply = @parseReply
		@req.on "message", () ->
			rep = parseReply arguments
			tagged = taggedRes[rep.tag]
			if rep.tag != null and tagged
				if tagged.type is "AUTH"
					tagged.fun rep
				else
					#TODO: error parameter for errs.
					#TODO: timeout in case.
					tagged.fun null, rep
				delete taggedRes[rep.tag]



	makeRequest: (method, uri, fn) ->
		auth = new Request @server, "AUTH", uri, @tag
		serv = @server
		taggedRes = @taggedRes
		socket = @req
		fun = (authres) ->
			console.log "Running auth method"
			if parseInt(authres.body) <= 1
				return fn(message: "banned", authres)
			req = new Request serv, method, uri, @tag
			req.setAuth authres.body
			req.makeRequest socket
			taggedRes[@tag++] =
				type: method
				fun: fn
			console.log "Request", req
		taggedRes[@tag] =
			type: "AUTH"
			fun: fun
		auth.tag = @tag++
		auth.makeRequest @req
		console.log "Auth:", auth


	parseReply: (args) ->
		code = 500
		body = ""
		type = "string"
		headers = {}
		index = 0
		tag = null
		_.each args, (arg) ->
			argm = arg.toString()
			if S(argm).startsWith "Code:"
				#TODO: Make safer
				code = S(argm).right(3).toInt()
			if S(argm).startsWith "Type:"
				type = S(argm).chompLeft("Type:").s
			if S(argm).startsWith "Tag:"
				#TODO: Make safer
				tag = S(argm).chompLeft("Tag:").toInt()
			if S(argm).startsWith "Header-"
				h = S(argm).chompLeft("Header-").s
				pos = h.indexOf ":"
				[key, val] = [h.slice(0,pos), h.slice(pos+1)]
				headers[key] = val
			if S(argm).startsWith "Body:"
				body = argm.slice("Body:".length)
		res = new Response code, body, type, headers, tag

class RequesterPool
	constructor: (@count, @server) ->
		@i = 0
		@reqs = []
		i = 0
		while i < @count
			@reqs.push new Requester @server
			i++

	req: () ->
		@i++
		@i = 0 if @i >= @count
		return @reqs[@i]

reqs = new RequesterPool 10, "tcp://192.168.5.200:13456"

#reqs.makeRequest "GET", "/demo/json", (err, res) ->
#	console.log err if err
#	console.log res


server = restify.createServer()
server.get '/', (req, res, next) ->
	reqs.req().makeRequest "GET", "/demo/json", (err, response) ->
		console.log err if err
		return next() if err

		res.send response.body
		next()


server.listen 8193


#deal = zmq.socket "dealer"
#deal.connect "tcp://localhost:13456"
#
#deal.on "message", () ->
#	parts = Array.apply null, arguments
#	console.log _.map(parts, (p) -> p.toString())
#
#deal.send ["Type:GET", "URI:/demo"]
#deal.send ["Type:GET", "URI:/demo/happy"]

