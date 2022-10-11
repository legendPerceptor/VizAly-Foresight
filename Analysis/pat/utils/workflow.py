""" This module contains the base Workflow class.
"""
import sys
import os
import json
import csv
from collections import OrderedDict

from pat.utils import file_utilities as futils
from pat.utils import plot_utilities as putils
from pat.utils import job as j



class Workflow(object):
    """ Class that describes workflow for Slurm scheduler.
    Note that jobs ordering appended to workflow has meaning in this simple API.
    """

    def __init__(self, name, json_data, workflow_dir=""):
        # store meta-data about the workflow
        self.name = name
        self.json_data = json_data

        self.json_path = self.json_data['project-home'] + self.json_data['wflow-path'] + "/" + self.name + ".json"

        # store a list of jobs in workflow
        self.jobs = []

        # location to store workflow files
        self.workflow_dir = workflow_dir

        # attributes for workflow construction
        self.submit_file = None

        # Add git tag
        git_tag = futils.get_git_version(self.json_data['foresight-home'])
        git_tag = git_tag.strip('\n')
        self.json_data["git-tag"] = git_tag



    def add_job(self, job, dependencies=None):
        """ Adds a job to the workflow.
        """
        self.jobs.append(job)




    def fill_input_files(self):
        """ Fill in  ["pat"]["input-files"]
        """
        base_path = self.json_data['project-home'] + self.json_data['wflow-path']

        # Remove all entries if any
        self.json_data['pat']['input-files'].clear()

        # Add the original
        orig_path_filename = futils.splitString(self.json_data['input']['filename'],'/')
        orig_item = {
            "output-prefix" : "orig",
            "path" : self.json_data['input']['filename']
        }
        self.json_data['pat']['input-files'].append(orig_item)

        # Add decompressed ones
        for _file in self.json_data['compressors']:
            json_item = {
                "output-prefix" : _file["output-prefix"],
                "path" : base_path + "/cbench/" + self.json_data['cbench']['output']['output-decompressed-location'] + "/" + _file['output-prefix'] + "__" + orig_path_filename[1]
            }

            self.json_data['pat']['input-files'].append(json_item)



    def add_cbench_job(self):
        """ Adds a CBench job to the workflow.
        """
    
        # Set up environment
        execute_dir = "cbench"

        if "configuration" in self.json_data["cbench"].keys():
            configurations = list(sum(self.json_data["cbench"]["configuration"].items(), ()))
        else:
            configurations = None

        if "evn_path" in self.json_data["cbench"]:
            environment =  self.json_data["foresight-home"] + self.json_data["cbench"]["evn_path"]
        else:
            environment = None

        # Find executable command
        exec_command = self.json_data["cbench"]["path"]



        # add a single CBench job to workflow for entire sweep
        cbench_job = j.Job(name="cbench",
                         execute_dir=execute_dir,
                         executable=exec_command,
                         arguments=[self.json_path],
                         configurations=configurations,
                         environment=environment)
        cbench_job.add_command("mkdir -p logs")
        self.add_job(cbench_job)


        # Fill in  ["pat"]["input-files"]
        self.fill_input_files()



    def add_analysis_jobs(self):
        """ Adds analysis jobs to workflow that do not produce final products.
        """
        raise NotImplementedError("Implement the `add_analysis` function to your workflow!")



    def add_cinema_plotting_jobs(self):
        """ Adds plotting jobs to workflow that produce final products.
        """
        raise NotImplementedError("Implement the `add_plotting_jobs` function to your workflow!")



    def write_submit(self):
        """ Writes Slurm workflow files.
        """

        # get foresight dir
        foresight_home = self.json_data["foresight-home"]

        # create workflow output dir
        # change to workflow output dir
        futils.create_folder(self.workflow_dir)
        os.chdir(self.workflow_dir)
    
        # write JSON data
        if not os.path.exists(os.path.dirname(self.json_path)):
            os.makedirs(os.path.dirname(self.json_path))
        with open(self.json_path, 'w') as fp:
            fp.write( json.dumps(self.json_data, indent=4) )
            fp.close()

        # create submission script
        self.submit_path = self.name + ".sh"
        with open(self.submit_path, "w") as fp:
            fp.write("#! /bin/bash\n")

        # loop over each job
        for i, job in enumerate(self.jobs):
    
            # get a unique name and index
            job.name = job.name if job.name != None else "job_{}".format(i)
            job._idx = i
    
            # figure out dependencies job indices
            parent_idxs = [j._idx for j in job._parents]
            if parent_idxs == []:
                depends_str = ""
            else:
                depends_str = "--dependency=afterok:" + ":".join(["$jid{}".format(j) for j in parent_idxs])
    
            # create workflow directory
            if not os.path.exists(job.execute_dir):
                os.makedirs(job.execute_dir)
    
            # write wrapper script for job
            path = job.execute_dir + "/" + job.name + ".sh"
            with open(path, "w") as fp:
                fp.write("#! /bin/bash\n")
                for key, val in zip(job.configurations[::2], job.configurations[1::2]):
                    fp.write("#SBATCH --{}={}\n".format(key, val))

                fp.write("date\n")
                fp.write("mkdir -p {}/{}\n".format(self.workflow_dir, job.execute_dir))
                fp.write("cd {}/{}\n".format(self.workflow_dir, job.execute_dir))

                for cmd in job.commands:
                    fp.write(cmd + "\n")

                if job.environment != None:
                    fp.write("source {}\n".format(job.environment))
                cmd = job.executable + " " + " ".join(map(str, job.arguments)) + "\n"
                cmd = cmd.replace("$foresight-home$", foresight_home)
                fp.write(cmd)
                fp.write("date\n")

            # append job to controller file
            slurm_out_path = job.execute_dir + "/" + job.name + ".slurm.out"
            with open(self.submit_path, "a") as fp:
                fp.write("\n# {}\n".format(job.name))
                fp.write("jid{}=$(sbatch {} --output {} {})\n".format(job._idx, depends_str, slurm_out_path, path))
                fp.write("jid{}=$(echo $jid{} | rev | cut -f 1 -d ' ' | rev)".format(job._idx, job._idx))
    

    def submit(self):
        """ Submits Slurm workflow.
        """
        os.system("bash {}".format(self.submit_path))


    def configuration_from_json_data(self, name):
        if "configuration" in self.json_data["pat"]["analysis-tool"]["analytics"][name].keys():
            configurations = list(sum(self.json_data["pat"]["analysis-tool"]["analytics"]
                                                    [name]["configuration"].items(), ()))
        else:
            configurations = None
        return configurations


    def environment_from_json_data(self):
        if "evn_path" in self.json_data["pat"].keys():
            env = self.json_data["pat"]["evn_path"]
        else:
            env = None
        return env
