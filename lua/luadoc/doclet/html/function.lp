<%
if module_doc then
	from = "modules/"..module_doc.name
elseif file_doc then
	from = "files/.."..file_doc.name
else
	from = ""
end
%>

<dt><%=func.private and "local " or ""%><a name="<%=func.name%>"></a>
<b><big><%=func.name%>&nbsp;</big></b>(<%=table.concat(func.param, ", ")%>)</dt>
<dd>

<%=func.description or ""%>

<%if func.release then%>
<br/><b>Release:</b> <%=func.release%>
<%end%>

<br/>

<%if type(func.param) == "table" and #func.param > 0 then%>
<b>Parameters</b>
<ul>
	<%for p = 1, #func.param do%>
	<li>
	  <%=func.param[p]%>: <%=func.param[func.param[p]] or ""%>
	</li>
	<%end%>
</ul>
<%end%>

<%if type(func.usage) == "string" then%>
<b>Usage:</b>
<ul>
	<li><%=func.usage%>
</ul>
<%elseif type(func.usage) == "table" then%>
<b>Usage</b>
<ul>
	<%for _, usage in ipairs(func.usage) do%>
	<li><%= usage %>
	<%end%>
</ul>
<%end%>

<%if type(func.ret) == "string" then%>
<b>Return value:</b>
<ul>
	<li><%=func.ret%>
</ul>
<%elseif type(func.ret) == "table" then%>
<b>Return values:</b>
<ol>
	<%for _, ret in ipairs(func.ret) do%>
	<li><%= ret %>
	<%end%>
</ol>
<%end%>

<%if type(func.see) == "string" then %>
<b>See also:</b>
<ul>
	<li><%=luadoc.doclet.mudlet.genLinkToFunction(doc, func.see)%>
</ul>
<%elseif type(func.see) == "table" and #func.see > 0 then %>
<b>See also:</b>
<ul>
	<%for i = 1, #func.see do%>
	<li><%=luadoc.doclet.mudlet.genLinkToFunction(doc, func.see[i])%>
	<%end%>
</ul>
<%end%>

</dd>
