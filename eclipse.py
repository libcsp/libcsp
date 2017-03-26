#! /usr/bin/env python
# encoding: utf-8
# Eclipse CDT 5.0 generator for Waf
# Richard Quirk 2009-1011 (New BSD License)
# Thomas Nagy 2011 (ported to Waf 1.6)

"""
Usage:

def options(opt):
	opt.load('eclipse')

$ waf configure eclipse
"""

import sys, os
from waflib import Utils, Logs, Context, Options, Build, TaskGen, Scripting
from xml.dom.minidom import Document

oe_cdt = 'org.eclipse.cdt'
cdt_mk = oe_cdt + '.make.core'
cdt_core = oe_cdt + '.core'
cdt_bld = oe_cdt + '.build.core'

class eclipse(Build.BuildContext):
	cmd = 'eclipse'
	fun = Scripting.default_cmd

	def execute(self):
		"""
		Entry point
		"""
		self.restore()
		if not self.all_envs:
			self.load_envs()
		self.recurse([self.run_dir])

		appname = getattr(Context.g_module, Context.APPNAME, os.path.basename(self.srcnode.abspath()))
		self.create_cproject(appname, pythonpath=self.env['ECLIPSE_PYTHON_PATH'])

	def create_cproject(self, appname, workspace_includes=[], pythonpath=[]):
		"""
		Create the Eclipse CDT .project and .cproject files
		@param appname The name that will appear in the Project Explorer
		@param build The BuildContext object to extract includes from
		@param workspace_includes Optional project includes to prevent
			  "Unresolved Inclusion" errors in the Eclipse editor
		@param pythonpath Optional project specific python paths
		"""
		source_dirs = []
		cpppath = self.env['CPPPATH']
		Logs.warn('Generating Eclipse CDT project files')

		for g in self.groups:
			for tg in g:
				if not isinstance(tg, TaskGen.task_gen):
					continue

				tg.post()
				if not getattr(tg, 'link_task', None):
					continue

				features = Utils.to_list(getattr(tg, 'features', ''))
				is_cc = 'c' in features or 'cxx' in features

				incnodes = tg.to_incnodes(tg.to_list(getattr(tg, 'includes', [])) + tg.env['INCLUDES'])
				for p in incnodes:
					path = p.path_from(self.srcnode)
					if path not in workspace_includes:
						workspace_includes.append(appname + '/' + path)

				#sources = Utils.to_list(getattr(tg, 'source', ''))
				#for p in sources:
				#	path = p.parent.path_from(self.srcnode)
				#	if is_cc and path not in source_dirs:
				#		source_dirs.append(path)

		project = self.impl_create_project(sys.executable, appname)
		self.srcnode.make_node('.project').write(project.toxml())

		waf = os.path.abspath(sys.argv[0])
		project = self.impl_create_cproject(sys.executable, waf, appname, workspace_includes, cpppath, source_dirs)
		self.srcnode.make_node('.cproject').write(project.toxml())

		if not os.path.exists('.settings'):
			os.makedirs('.settings')

		settings = self.impl_create_settings(sys.executable, waf, appname, workspace_includes, cpppath, source_dirs)
		self.srcnode.make_node('.settings/language.settings.xml').write(settings.toxml())

		#project = self.impl_create_pydevproject(appname, sys.path, pythonpath)
		#self.srcnode.make_node('.pydevproject').write(project.toxml())

	def impl_create_project(self, executable, appname):
		doc = Document()
		projectDescription = doc.createElement('projectDescription')
		self.add(doc, projectDescription, 'name', appname)
		self.add(doc, projectDescription, 'comment')
		self.add(doc, projectDescription, 'projects')
		buildSpec = self.add(doc, projectDescription, 'buildSpec')
		buildCommand = self.add(doc, buildSpec, 'buildCommand')
		self.add(doc, buildCommand, 'name', oe_cdt + '.managedbuilder.core.genmakebuilder')
		self.add(doc, buildCommand, 'triggers', 'clean,full,incremental,')
		arguments = self.add(doc, buildCommand, 'arguments')

		# the default make-style targets are overwritten by the .cproject values
		dictionaries = {
				cdt_mk + '.contents': cdt_mk + '.activeConfigSettings',
				cdt_mk + '.enableAutoBuild': 'false',
				cdt_mk + '.enableCleanBuild': 'true',
				cdt_mk + '.enableFullBuild': 'true',
				}
		for k, v in dictionaries.items():
			self.addDictionary(doc, arguments, k, v)

		buildCommand = self.add(doc, buildSpec, 'buildCommand')
		self.add(doc, buildCommand, 'name', 'org.eclipse.cdt.managedbuilder.core.ScannerConfigBuilder')
		self.add(doc, buildCommand, 'triggers', 'full,incremental,')
		self.add(doc, buildCommand, 'arguments')
		
		natures = self.add(doc, projectDescription, 'natures')

		self.add(doc, natures, 'nature', 'org.python.pydev.pythonNature')
		self.add(doc, natures, 'nature', 'org.eclipse.cdt.managedbuilder.core.ScannerConfigNature')
		self.add(doc, natures, 'nature', 'org.eclipse.cdt.managedbuilder.core.managedBuildNature')
		self.add(doc, natures, 'nature', 'org.eclipse.cdt.core.cnature')
		self.add(doc, natures, 'nature', 'org.eclipse.cdt.core.ccnature')

		doc.appendChild(projectDescription)
		return doc

	def impl_create_cproject(self, executable, waf, appname, workspace_includes, cpppath, source_dirs=[]):
		doc = Document()
		doc.appendChild(doc.createProcessingInstruction('fileVersion', '4.0.0'))
		cconf_id = cdt_core + '.default.config.1'
		cproject = doc.createElement('cproject')
		storageModule = self.add(doc, cproject, 'storageModule',
				{'moduleId': cdt_core + '.settings'})
		cconf = self.add(doc, storageModule, 'cconfiguration', {'id':cconf_id})

		storageModule = self.add(doc, cconf, 'storageModule',
				{'buildSystemId': oe_cdt + '.managedbuilder.core.configurationDataProvider',
				 'id': cconf_id,
				 'moduleId': cdt_core + '.settings',
				 'name': 'Default'})

		self.add(doc, storageModule, 'externalSettings')

		extensions = self.add(doc, storageModule, 'extensions')
		extension_list = """
			VCErrorParser
			MakeErrorParser
			GCCErrorParser
			GASErrorParser
			GLDErrorParser
		""".split()
		ext = self.add(doc, extensions, 'extension',
					{'id': cdt_core + '.ELF', 'point':cdt_core + '.BinaryParser'})
		for e in extension_list:
			ext = self.add(doc, extensions, 'extension',
					{'id': cdt_core + '.' + e, 'point':cdt_core + '.ErrorParser'})

		storageModule = self.add(doc, cconf, 'storageModule',
				{'moduleId': 'cdtBuildSystem', 'version': '4.0.0'})
		config = self.add(doc, storageModule, 'configuration',
					{'artifactName': appname,
					 'id': cconf_id,
					 'name': 'Default',
					 'parent': cdt_bld + '.prefbase.cfg'})
		folderInfo = self.add(doc, config, 'folderInfo',
							{'id': cconf_id+'.', 'name': '/', 'resourcePath': ''})

		toolChain = self.add(doc, folderInfo, 'toolChain',
				{'id': cdt_bld + '.prefbase.toolchain.1',
				 'name': 'No ToolChain',
				 'resourceTypeBasedDiscovery': 'false',
				 'superClass': cdt_bld + '.prefbase.toolchain'})

		targetPlatform = self.add(doc, toolChain, 'targetPlatform',
				{ 'binaryParser': 'org.eclipse.cdt.core.ELF',
				  'id': cdt_bld + '.prefbase.toolchain.1', 'name': ''})

		waf_build = 'waf %s'%(eclipse.fun)
		waf_clean = 'waf clean'
		builder = self.add(doc, toolChain, 'builder',
						{'autoBuildTarget': waf_build,
						 'command': executable,
						 'enableAutoBuild': 'false',
						 'cleanBuildTarget': waf_clean,
						 'enableIncrementalBuild': 'true',
						 'id': cdt_bld + '.settings.default.builder.1',
						 'incrementalBuildTarget': waf_build,
						 'managedBuildOn': 'false',
						 'name': 'Gnu Make Builder',
						 'superClass': cdt_bld + '.settings.default.builder'})

		for tool_name in ("Assembly", "GNU C", "GNU C++"):
			tool = self.add(doc, toolChain, 'tool',
					{'id': cdt_bld + '.settings.holder.1',
					 'name': tool_name,
					 'superClass': cdt_bld + '.settings.holder'})
			if cpppath or workspace_includes:
				incpaths = cdt_bld + '.settings.holder.incpaths'
				option = self.add(doc, tool, 'option',
						{'id': incpaths+'.1',
						 'name': 'Include Paths',
						 'superClass': incpaths,
						 'valueType': 'includePath'})
				for i in workspace_includes:
					self.add(doc, option, 'listOptionValue',
								{'builtIn': 'false',
								'value': '"${workspace_loc:/%s}"'%(i)})
				for i in cpppath:
					self.add(doc, option, 'listOptionValue', {'builtIn': 'false', 'value': '"%s"'%(i)})

			if tool_name == "Assembly":
				input_type = self.add(doc, tool, 'inputType', 
						{'id' : cdt_bld + '.settings.holder.inType.1', 
						 'languageId' : 'org.eclipse.cdt.core.assembly',
						 'languageName' : 'Assembly',
						 'sourceContentType' : 'org.eclipse.cdt.core.asmSource',
						 'superClass' : cdt_bld + 'settings.holder.inType'})


			if tool_name == "GNU C":
				input_type = self.add(doc, tool, 'inputType', 
						{'id' : cdt_bld + '.settings.holder.inType.2', 
						 'languageId' : 'org.eclipse.cdt.core.gcc',
						 'languageName' : 'GNU C',
						 'sourceContentType' : 'org.eclipse.cdt.core.cSource,org.eclipse.cdt.core.cHeader',
						 'superClass' : cdt_bld + 'settings.holder.inType'})

			if tool_name == "GNU C++":
				input_type = self.add(doc, tool, 'inputType', 
						{'id' : cdt_bld + '.settings.holder.inType.3', 
						 'languageId' : 'org.eclipse.cdt.core.g++',
						 'languageName' : 'GNU C++',
						 'sourceContentType' : 'org.eclipse.cdt.core.cxxSource,org.eclipse.cdt.core.cxxHeader',
						 'superClass' : cdt_bld + 'settings.holder.inType'})


		if source_dirs:
			sourceEntries = self.add(doc, config, 'sourceEntries')
			for i in source_dirs:
				 self.add(doc, sourceEntries, 'entry',
							{'excluding': i,
							'flags': 'VALUE_WORKSPACE_PATH|RESOLVED',
							'kind': 'sourcePath',
							'name': ''})
				 self.add(doc, sourceEntries, 'entry',
							{
							'flags': 'VALUE_WORKSPACE_PATH|RESOLVED',
							'kind': 'sourcePath',
							'name': i})

		storageModule = self.add(doc, cconf, 'storageModule',
							{'moduleId': cdt_mk + '.buildtargets'})
		buildTargets = self.add(doc, storageModule, 'buildTargets')
		def addTargetWrap(name, runAll):
			return self.addTarget(doc, buildTargets, executable, name, 'waf %s'%(name), runAll)
		addTargetWrap('configure', True)
		addTargetWrap('dist', False)
		addTargetWrap('install', False)
		addTargetWrap('check', False)

		storageModule = self.add(doc, cproject, 'storageModule',
							{'moduleId': 'cdtBuildSystem',
							 'version': '4.0.0'})

		

		project = self.add(doc, storageModule, 'project',
					{'id': '%s.null.1'%appname, 'name': appname})
		# Add DoxyGen comment parser
		storageModule = self.add(doc, cproject, 'storageModule', {'moduleId': 'org.eclipse.cdt.internal.ui.text.commentOwnerProjectMappings'})
		docCommentOwner = self.add(doc, storageModule, 'doc-comment-owner', {'id' : 'org.eclipse.cdt.ui.doxygen'})
		path = self.add(doc, docCommentOwner, 'path', {'value' : ''})

		# Add scanner info generator
		storageModule = self.add(doc, cproject, 'storageModule', {'moduleId': 'scannerConfiguration'})
		self.add(doc, storageModule, 'autodiscovery', {'enabled' : 'true', 'problemReportingEnabled' : 'true', 'selectedProfileId' : 'org.eclipse.cdt.make.core.GCCStandardMakePerProjectProfile'})
		scannerConfigBuildInfo = self.add(doc, storageModule, 'scannerConfigBuildInfo', {'instanceId' : cconf_id})
		self.add(doc, scannerConfigBuildInfo, 'autodiscovery', {'enabled' : 'true', 'problemReportingEnabled' : 'true', 'selectedProfileId' : 'org.eclipse.cdt.make.core.GCCStandardMakePerProjectProfile'})
		profile = self.add(doc, scannerConfigBuildInfo, 'profile', {'id' : 'org.eclipse.cdt.make.core.GCCStandardMakePerProjectProfile'})
		buildOutputProvider = self.add(doc, profile, 'buildOutputProvider', {})
		self.add(doc, buildOutputProvider, 'openAction', {'enabled' : 'true', 'filePath' : ''})
		self.add(doc, buildOutputProvider, 'parser', {'enabled' : 'true'})
		scannerInfoProvider = self.add(doc, profile, 'scannerInfoProvider', {'id' : 'specsFile'})

		# Generate compiler string
		if type(self.env.CC) == list:
			cc_str = self.env.CC[0]
		else:
			cc_str = self.env.CC

		# Generate CFLAGS
		ccflags_str = ''
		for flag in self.env.CFLAGS:
			ccflags_str = ccflags_str + ' ' + flag

		self.add(doc, scannerInfoProvider, 'runAction', {'arguments' : ccflags_str + ' -E -P -v -dD ${plugin_state_location}/${specs_file}', 'command' : cc_str, 'useDefault' : 'true'})
		self.add(doc, scannerInfoProvider, 'parser', {'enabled' : 'true'})

		# Generate list of sources
		sources = list()
		for group in self.groups:
			for task in group:
				#print(task)
				#print(task.source)
				sources += task.source

		files = self.srcnode.ant_glob('**/*.c', excl='/build')
		files = filter(lambda x:x not in sources, files)

		#print(sources)
		#print(len(sources))		
		#print(files)
		#print(len(files))

		excludes = '|'.join([fil.srcpath() for fil in files])

		sourceEntries = self.add(doc, config, 'sourceEntries', {});
		self.add(doc, sourceEntries, 'entry', {'excluding' : excludes , 'flags' : 'VALUE_WORKSPACE_PATH', 'kind' : 'sourcePath', 'name' : ''});

		doc.appendChild(cproject)
		return doc

	def impl_create_settings(self, executable, waf, appname, workspace_includes, cpppath, source_dirs=[]):

		# Generate compiler string
		if type(self.env.CC) == list:
			cc_str = self.env.CC[0]
		else:
			cc_str = self.env.CC

		# Generate CFLAGS
		ccflags_str = ''
		for flag in self.env.CFLAGS:
			ccflags_str = ccflags_str + ' ' + flag

		doc = Document()
		project = doc.createElement('project')
		configuration = self.add(doc, project, 'configuration', {'id' : 'org.eclipse.cdt.core.default.config.1', 'name' : 'default'})
		extension = self.add(doc, configuration, 'extension', {'point' : 'org.eclipse.cdt.core.LanguageSettingsProvider'})
		provider1 = self.add(doc, extension, 'provider', {'copy-of' : 'extension', 'id' : 'org.eclipse.cdt.ui.UserLanguageSettingsProvider'})
		#providerReference = self.add(doc, extension, 'provider-reference', {'id' : 'org.eclipse.cdt.managedbuilder.core.MBSLanguageSettingsProvider', 'ref' : 'shared-provider'})
		provider2 = self.add(doc, extension, 'provider', {
			'class' : 'org.eclipse.cdt.managedbuilder.language.settings.providers.GCCBuiltinSpecsDetector',
			'console' : 'false',
			'id' : 'org.eclipse.cdt.managedbuilder.core.GCCBuiltinSpecsDetector',
			'keep-relative-paths' : 'false',
			'env-hash' : '857849289',
			'name' : 'CDT GCC Built-in Compiler Settings',
			'parameter' : cc_str + ccflags_str + ' -E -P -v -dD "${INPUTS}"'})
		langscope1 = self.add(doc, provider2, 'language-scope', {'id' : 'org.eclipse.cdt.core.gcc'})
		langscope2 = self.add(doc, provider2, 'language-scope', {'id' : 'org.eclipse.cdt.core.g++'})

		doc.appendChild(project)
		return doc

	def impl_create_pydevproject(self, appname, system_path, user_path):
		# create a pydevproject file
		doc = Document()
		doc.appendChild(doc.createProcessingInstruction('eclipse-pydev', 'version="1.0"'))
		pydevproject = doc.createElement('pydev_project')
		prop = self.add(doc, pydevproject,
					   'pydev_property',
					   'python %d.%d'%(sys.version_info[0], sys.version_info[1]))
		prop.setAttribute('name', 'org.python.pydev.PYTHON_PROJECT_VERSION')
		prop = self.add(doc, pydevproject, 'pydev_property', 'Default')
		prop.setAttribute('name', 'org.python.pydev.PYTHON_PROJECT_INTERPRETER')
		# add waf's paths
		wafadmin = [p for p in system_path if p.find('wafadmin') != -1]
		if wafadmin:
			prop = self.add(doc, pydevproject, 'pydev_pathproperty',
					{'name':'org.python.pydev.PROJECT_EXTERNAL_SOURCE_PATH'})
			for i in wafadmin:
				self.add(doc, prop, 'path', i)
		if user_path:
			prop = self.add(doc, pydevproject, 'pydev_pathproperty',
					{'name':'org.python.pydev.PROJECT_SOURCE_PATH'})
			for i in user_path:
				self.add(doc, prop, 'path', '/'+appname+'/'+i)

		doc.appendChild(pydevproject)
		return doc

	def addDictionary(self, doc, parent, k, v):
		dictionary = self.add(doc, parent, 'dictionary')
		self.add(doc, dictionary, 'key', k)
		self.add(doc, dictionary, 'value', v)
		return dictionary

	def addTarget(self, doc, buildTargets, executable, name, buildTarget, runAllBuilders=True):
		target = self.add(doc, buildTargets, 'target', {'name': name, 'path': '', 'targetID': oe_cdt + '.build.MakeTargetBuilder'})
		self.add(doc, target, 'buildCommand', executable)
		self.add(doc, target, 'buildArguments', None)
		self.add(doc, target, 'buildTarget', buildTarget)
		self.add(doc, target, 'stopOnError', 'true')
		self.add(doc, target, 'useDefaultCommand', 'false')
		self.add(doc, target, 'runAllBuilders', str(runAllBuilders).lower())

	def add(self, doc, parent, tag, value = None):
		el = doc.createElement(tag)
		if (value):
			if type(value) == type(str()):
				el.appendChild(doc.createTextNode(value))
			elif type(value) == type(dict()):
				self.setAttributes(el, value)
		parent.appendChild(el)
		return el

	def setAttributes(self, node, attrs):
		for k, v in attrs.items():
			node.setAttribute(k, v)

