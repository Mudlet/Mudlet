<%
if module_doc then
	from = "modules/"..module_doc.name
elseif file_doc then
	from = "files/.."..file_doc.name
else
	from = ""
end
%>

<dt><%=func.private and "local " or ""%><a name="<%=func.name%>"></a><strong><%=func.name%></strong>&nbsp;(<%=table.concat(func.param, ", ")%>)</dt>
<dd>

<%if func.filename then%>
<i>from <%=func.filename%></i>
<br/><br/>
<%end%>

<%=func.description or ""%>

<%if func.release then%>
<br/><b>Release:</b> <%=func.release%>
<%end%>

<br/><br/>

<%if type(func.param) == "table" and #func.param > 0 then%>
<h3>Parameters</h3>
<ul>
	<%for p = 1, #func.param do%>
	<li>
	  <%=func.param[p]%>: <%=func.param[func.param[p]] or ""%>
	</li>
	<%end%>
</ul>
<%end%>

<%if type(func.usage) == "string" then%>
<h3>Usage:</h3>
<ul>
	<li><%=func.usage%>
</ul>
<%elseif type(func.usage) == "table" then%>
<h3>Usage</h3>
<ul>
	<%for _, usage in ipairs(func.usage) do%>
	<li><%= usage %>
	<%end%>
</ul>
<%end%>

<%if type(func.ret) == "string" then%>
<h3>Return value:</h3>
<ul>
	<li><%=func.ret%>
</ul>
<%elseif type(func.ret) == "table" then%>
<h3>Return values:</h3>
<ol>
	<%for _, ret in ipairs(func.ret) do%>
	<li><%= ret %>
	<%end%>
</ol>
<%end%>

<%if type(func.see) == "string" then %>
<h3>See also:</h3>
<ul>
	<li><a href="#<%=func.see%>"><%=func.see%></a>
</ul>
<%elseif type(func.see) == "table" and #func.see > 0 then %>
<h3>See also:</h3>
<ul>
	<%for i = 1, #func.see do%>
	<li><a href="#<%=func.see[i]%>"><%=func.see[i]%></a>
	<%end%>
</ul>
<%end%>

</dd>
