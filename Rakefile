#!/usr/bin/env ruby

require 'pathname'
$root = Pathname(__FILE__).dirname

require 'rubygems'
require 'rake'
require 'rake/clean'
require 'rake/packagetask'
require 'rake/gempackagetask'
require 'rake/testtask'
require 'rake/rdoctask'

task :spec do
  load $root.join('spec', 'spec_base.rb')
end
task :default => [ :spec ]

CLEAN.include('pkg', 'tmp')

gemspec = Gem::Specification.new do |s|
  s.name = 'tokyomessenger'
  s.version = '0.5'
  s.authors = [ 'Matt Bauer' ]
  s.email = 'bauer@pedalbrain.com'
  s.homepage = 'http://github.com/mattbauer/tokyomessenger/'
  s.platform = Gem::Platform::RUBY
  s.summary = 'A C based TokyoTyrant Ruby adapter that doesn\'t require TokyoCabinet or TokyoTyrant to be installed'
  s.require_path = 'ext'
  s.test_file = 'spec/spec.rb'
  s.has_rdoc = true
  s.extra_rdoc_files = %w{ README.rdoc }

  s.files = ['COPYING',
             'Rakefile',
             'README.rdoc'] +
             Dir['ext/**/*'] +
             Dir['lib/**/*.rb'] +
             Dir['spec/**/*'] +
             Dir['benchmarks/**/*']
  s.extensions << "ext/extconf.rb"

  s.add_runtime_dependency('fast_hash_ring', '>= 0.1.1')
end

task :gemspec do
  File.open('tokyomessenger.gemspec', 'w') do |f|
    f.write(gemspec.to_ruby)
  end
end

Rake::GemPackageTask.new(gemspec) do |pkg|
  pkg.need_tar = true
end

Rake::PackageTask.new('heroku-tokyomessenger', '0.5') do |pkg|
  pkg.need_zip = true
  pkg.package_files = FileList[
    'COPYING',
    'Rakefile',
    'README.rdoc',
    'ext/**/*',
    'lib/**/*.[rb]',
    'spec/**/*',
    'benchmarks/**/*'
  ].to_a
  class << pkg
    def package_name
      "#{@name}-#{@version}-src"
    end
  end
end
